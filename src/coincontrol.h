// Copyright (c) 2013-2014 Dogecoin Developers
// Enhanced with fee estimation features

#ifndef COINCONTROL_H
#define COINCONTROL_H

// Coin control defaults
static const int DEFAULT_CONFIRM_TARGET = 6;     // Default: 6 blocks (~15 minutes for 2.5 min blocks)
static const int64 DEFAULT_MIN_FEE_RATE = 1000;  // Minimum 1000 satoshis per KB

/** Coin Control Features with Advanced Fee Analysis. */
class CCoinControl
{
public:
    CTxDestination destChange;
    
    // Fee control options
    bool fUseCustomFee;           //!< Use custom fee instead of estimation
    int64 nCustomFeeRate;         //!< Custom fee rate in satoshis per KB
    int nConfirmTarget;           //!< Desired confirmation time in blocks
    bool fAllowOtherInputs;       //!< Allow adding other inputs if needed
    bool fAllowWatchOnly;         //!< Allow selecting watch-only addresses
    
    // Fee estimation results (filled by wallet)
    int64 nEstimatedFee;          //!< Estimated fee for transaction
    int64 nEstimatedFeeRate;      //!< Estimated fee rate used
    int nEstimatedConfTime;       //!< Estimated confirmation time in blocks
    unsigned int nEstimatedSize;  //!< Estimated transaction size in bytes

    CCoinControl()
    {
        SetNull();
    }
        
    void SetNull()
    {
        destChange = CNoDestination();
        setSelected.clear();
        fUseCustomFee = false;
        nCustomFeeRate = 0;
        nConfirmTarget = DEFAULT_CONFIRM_TARGET;
        fAllowOtherInputs = true;
        fAllowWatchOnly = false;
        nEstimatedFee = 0;
        nEstimatedFeeRate = 0;
        nEstimatedConfTime = 0;
        nEstimatedSize = 0;
    }
    
    bool HasSelected() const
    {
        return (setSelected.size() > 0);
    }
    
    bool IsSelected(const uint256& hash, unsigned int n) const
    {
        COutPoint outpt(hash, n);
        return (setSelected.count(outpt) > 0);
    }
    
    void Select(COutPoint& output)
    {
        setSelected.insert(output);
    }
    
    void UnSelect(COutPoint& output)
    {
        setSelected.erase(output);
    }
    
    void UnSelectAll()
    {
        setSelected.clear();
    }

    void ListSelected(std::vector<COutPoint>& vOutpoints)
    {
        vOutpoints.assign(setSelected.begin(), setSelected.end());
    }
    
    /** Get number of selected coins */
    size_t GetSelectedCount() const
    {
        return setSelected.size();
    }
    
    /** Set custom fee rate (satoshis per KB) */
    void SetCustomFeeRate(int64 nFeeRate)
    {
        fUseCustomFee = true;
        nCustomFeeRate = nFeeRate;
    }
    
    /** Set confirmation target in blocks */
    void SetConfirmTarget(int nBlocks)
    {
        nConfirmTarget = nBlocks;
        if (nConfirmTarget < 1) nConfirmTarget = 1;
        if (nConfirmTarget > 25) nConfirmTarget = 25;
    }
    
    /** Clear custom fee, use estimation */
    void UseEstimatedFee()
    {
        fUseCustomFee = false;
        nCustomFeeRate = 0;
    }
        
private:
    std::set<COutPoint> setSelected;

};

#endif // COINCONTROL_H
