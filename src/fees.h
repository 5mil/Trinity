// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_FEES_H
#define BITCOIN_FEES_H

#include "uint256.h"
#include "sync.h"

#include <map>
#include <string>

class CBlockIndex;
class CTransaction;

/** 
 * We want to be able to estimate fees that are needed for
 * a transaction to be included in a certain number of blocks.
 * This is based on recent transaction activity in the network.
 */

/** Enumeration of reason for returned fee estimate */
enum FeeReason {
    NONE,
    HALF_ESTIMATE,      //!< Only half the required data available  
    FULL_ESTIMATE,      //!< Full estimate from sufficient data
    DEFAULT_ESTIMATE,   //!< Default fallback fee
    REQUIRED_ESTIMATE,  //!< Required for relay/mining
};

/** 
 * Estimate fee rate needed to get a transaction confirmed within
 * confTarget blocks. Uses statistics from recent blocks to estimate.
 */
class CFeeRate;

/** Fee estimation class */
class CBlockPolicyEstimator
{
private:
    /** Track transactions in blocks to gather statistics */
    std::map<int, int64> mapBlockFees;  // Block height -> median fee rate
    std::map<int, int> mapBlockTxCount;  // Block height -> tx count
    
    mutable CCriticalSection cs_feeEstimator;
    
    /** Minimum fee rate (satoshis per KB) */
    int64 nMinFeeRate;
    
    /** Default fee rate when no data available */
    int64 nDefaultFeeRate;
    
    /** How many blocks to keep statistics for */
    static const int HISTORY_BLOCKS = 25;
    
    /** Calculate median fee from recent blocks */
    int64 CalculateMedianFee(int nBlocks) const;
    
public:
    CBlockPolicyEstimator(int64 minFeeRate);
    
    /** Process a new block to update fee estimates */
    void ProcessBlock(int nBlockHeight, const std::vector<const CTransaction*>& vtx);
    
    /** Estimate fee rate needed to confirm within nBlocks */
    int64 EstimateFee(int nBlocks) const;
    
    /** Estimate confirmation time for a given fee rate (returns number of blocks) */
    int EstimateConfirmationTime(int64 nFeeRate) const;
    
    /** Get minimum fee rate */
    int64 GetMinFeeRate() const { return nMinFeeRate; }
    
    /** Write state to disk */
    void Write();
    
    /** Read state from disk */
    void Read();
};

/** Enhanced coin control with fee analysis */
struct CCoinControlFeatures
{
    /** Target confirmation time in blocks */
    int nConfTarget;
    
    /** Custom fee rate (0 = use estimation) */
    int64 nCustomFeeRate;
    
    /** Minimum acceptable fee rate */
    int64 nMinFeeRate;
    
    /** Calculate optimal fee for transaction */
    int64 GetOptimalFee(size_t nTxSize) const;
    
    /** Estimate confirmation time for given fee */
    int EstimateConfTime(int64 nFeeRate) const;
    
    CCoinControlFeatures()
    {
        SetNull();
    }
    
    void SetNull()
    {
        nConfTarget = 6;  // Default: confirm within 6 blocks (~1 hour)
        nCustomFeeRate = 0;
        nMinFeeRate = 1000;  // 1000 satoshis per KB minimum
    }
};

/** Global fee estimator */
extern CBlockPolicyEstimator feeEstimator;

/** Helper function to format fee display */
std::string FormatFeeRate(int64 nFeeRate);

/** Helper function to estimate transaction size */
unsigned int EstimateTxSize(unsigned int nInputs, unsigned int nOutputs);

#endif // BITCOIN_FEES_H
