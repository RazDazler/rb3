#pragma once
#include "Tracker.h"
#include "game/TrackerDisplay.h"

class OverdriveTimeTracker : public Tracker {
public:
    OverdriveTimeTracker(TrackerSource *, TrackerBandDisplay &, TrackerBroadcastDisplay &);
    virtual ~OverdriveTimeTracker();
    virtual void UpdateGoalValueLabel(UILabel &) const;
    virtual void UpdateCurrentValueLabel(UILabel &) const;
    virtual String GetPlayerContributionString(Symbol) const;
    virtual void FirstFrame_(float);
    virtual void Poll_(float);
#ifdef VERSION_SZBE69_B8
    virtual void TargetSuccess(int) const;
#else
    virtual void TargetSuccess(int) const {}
#endif
#ifdef VERSION_SZBE69_B8
    virtual DataArrayPtr GetTargetDescription(int idx) const;
    virtual float GetCurrentValue() const;
#else
    virtual DataArrayPtr GetTargetDescription(int idx) const {
        return TrackerDisplay::MakeTimeTargetDescription(mTargets[idx]);
    }
    virtual float GetCurrentValue() const { return unk60; }
#endif
    virtual void SavePlayerStats() const;

    void UpdateTimeRemainingDisplay();

    float unk58;
    float unk5c;
    float unk60;
    float unk64;
    int unk68;
    bool unk6c;
};