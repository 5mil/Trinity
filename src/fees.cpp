// Copyright (c) 2009-2014 The Bitcoin developers  
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "fees.h"
#include "core.h"
#include "main.h"
#include "util.h"

#include <algorithm>
#include <vector>

using namespace std;

// Fee estimation constants
static const int64 DEFAULT_MIN_FEE_RATE = 1000;    // Minimum fee rate (1000 sat/KB)
static const int64 DEFAULT_FEE_RATE = 20000;       // Default fee rate (20000 sat/KB)

// Block confirmation targets
static const int FEE_CONFIRM_NEXT_BLOCK = 1;
static const int FEE_CONFIRM_FAST = 3;
static const int FEE_CONFIRM_STANDARD = 6;
static const int FEE_CONFIRM_ECONOMY = 12;

// Fee multipliers (as fractions: numerator/denominator)
static const int FEE_PREMIUM_NUM = 3;
static const int FEE_PREMIUM_DEN = 2;      // 1.5x for next block
static const int FEE_HIGH_NUM = 5;
static const int FEE_HIGH_DEN = 4;         // 1.25x for fast confirmation
static const int FEE_LOW_NUM = 3;
static const int FEE_LOW_DEN = 4;          // 0.75x for economy

CBlockPolicyEstimator::CBlockPolicyEstimator(int64 minFeeRate)
    : nMinFeeRate(minFeeRate), nDefaultFeeRate(DEFAULT_FEE_RATE)
{
}

void CBlockPolicyEstimator::ProcessBlock(int nBlockHeight, const std::vector<const CTransaction*>& vtx)
{
    LOCK(cs_feeEstimator);
    
    if (vtx.empty())
        return;
    
    // Calculate fees for this block
    int64 nTotalFees = 0;
    int nTxCount = 0;
    
    for (vector<const CTransaction*>::const_iterator it = vtx.begin(); it != vtx.end(); ++it)
    {
        const CTransaction& tx = **it;
        
        // Skip coinbase
        if (tx.IsCoinBase())
            continue;
            
        // For now, we'll use a simple heuristic
        // In a real implementation, we'd track actual fees paid
        nTxCount++;
    }
    
    if (nTxCount > 0) {
        // Store block statistics
        mapBlockTxCount[nBlockHeight] = nTxCount;
        
        // Clean up old data
        if (mapBlockTxCount.size() > HISTORY_BLOCKS) {
            int nOldestBlock = nBlockHeight - HISTORY_BLOCKS;
            mapBlockTxCount.erase(nOldestBlock);
            mapBlockFees.erase(nOldestBlock);
        }
    }
}

int64 CBlockPolicyEstimator::CalculateMedianFee(int nBlocks) const
{
    LOCK(cs_feeEstimator);
    
    if (mapBlockFees.empty())
        return nDefaultFeeRate;
    
    vector<int64> vFees;
    for (map<int, int64>::const_iterator it = mapBlockFees.begin(); 
         it != mapBlockFees.end(); ++it)
    {
        vFees.push_back(it->second);
    }
    
    if (vFees.empty())
        return nDefaultFeeRate;
    
    sort(vFees.begin(), vFees.end());
    
    // Return median
    return vFees[vFees.size() / 2];
}

int64 CBlockPolicyEstimator::EstimateFee(int nBlocks) const
{
    LOCK(cs_feeEstimator);
    
    // Sanity check
    if (nBlocks <= 0)
        nBlocks = 1;
    if (nBlocks > 25)
        nBlocks = 25;
    
    // If we don't have enough data, return default
    if (mapBlockFees.size() < 3)
        return nDefaultFeeRate;
    
    // Calculate median fee from recent blocks
    int64 nMedianFee = CalculateMedianFee(nBlocks);
    
    // Adjust based on confirmation target
    // Faster confirmation = higher fee
    int64 nEstimate = nMedianFee;
    
    if (nBlocks <= FEE_CONFIRM_NEXT_BLOCK) {
        // Next block - add premium
        nEstimate = nMedianFee * FEE_PREMIUM_NUM / FEE_PREMIUM_DEN;
    } else if (nBlocks <= FEE_CONFIRM_FAST) {
        // Fast confirmation - add moderate premium
        nEstimate = nMedianFee * FEE_HIGH_NUM / FEE_HIGH_DEN;
    } else if (nBlocks <= FEE_CONFIRM_STANDARD) {
        // Standard confirmation - use median
        nEstimate = nMedianFee;
    } else {
        // Economy confirmation - can use lower fee
        nEstimate = nMedianFee * FEE_LOW_NUM / FEE_LOW_DEN;
    }
    
    // Ensure minimum
    if (nEstimate < nMinFeeRate)
        nEstimate = nMinFeeRate;
    
    return nEstimate;
}

int CBlockPolicyEstimator::EstimateConfirmationTime(int64 nFeeRate) const
{
    LOCK(cs_feeEstimator);
    
    // If fee rate is very low, expect long wait
    if (nFeeRate < nMinFeeRate)
        return 25;  // Maximum blocks we track
    
    int64 nMedianFee = CalculateMedianFee(FEE_CONFIRM_STANDARD);
    
    // Estimate confirmation time based on fee rate relative to median
    if (nFeeRate >= nMedianFee * FEE_PREMIUM_NUM / FEE_PREMIUM_DEN) {
        return FEE_CONFIRM_NEXT_BLOCK;
    } else if (nFeeRate >= nMedianFee * FEE_HIGH_NUM / FEE_HIGH_DEN) {
        return FEE_CONFIRM_FAST;
    } else if (nFeeRate >= nMedianFee) {
        return FEE_CONFIRM_STANDARD;
    } else if (nFeeRate >= nMedianFee * FEE_LOW_NUM / FEE_LOW_DEN) {
        return FEE_CONFIRM_ECONOMY;
    } else {
        return 25; // More than 25 blocks
    }
}

void CBlockPolicyEstimator::Write()
{
    // TODO: Implement persistence
}

void CBlockPolicyEstimator::Read()
{
    // TODO: Implement persistence
}

// CCoinControlFeatures implementation

int64 CCoinControlFeatures::GetOptimalFee(size_t nTxSize) const
{
    // If custom fee is set, use it
    if (nCustomFeeRate > 0) {
        return (nCustomFeeRate * nTxSize) / 1000;
    }
    
    // Otherwise, estimate based on confirmation target
    int64 nFeeRate = feeEstimator.EstimateFee(nConfTarget);
    
    // Calculate total fee
    int64 nFee = (nFeeRate * nTxSize) / 1000;
    
    // Ensure minimum
    int64 nMinFee = (nMinFeeRate * nTxSize) / 1000;
    if (nFee < nMinFee)
        nFee = nMinFee;
    
    return nFee;
}

int CCoinControlFeatures::EstimateConfTime(int64 nFeeRate) const
{
    return feeEstimator.EstimateConfirmationTime(nFeeRate);
}

// Global fee estimator instance
CBlockPolicyEstimator feeEstimator(1000);

// Helper functions

std::string FormatFeeRate(int64 nFeeRate)
{
    // Format as satoshis per KB
    return strprintf("%d sat/KB", nFeeRate);
}

unsigned int EstimateTxSize(unsigned int nInputs, unsigned int nOutputs)
{
    /**
     * Estimate transaction size in bytes:
     * - Each input: ~180 bytes (148 bytes + overhead)
     * - Each output: ~34 bytes
     * - Transaction overhead: ~10 bytes
     */
    return nInputs * 180 + nOutputs * 34 + 10;
}
