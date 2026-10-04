#include "game/VocalPart.h"
#include "decomp.h"

#if defined(VERSION_SZBE69_B8)
// Preserve original literal ordering while incomplete methods remain.
DECOMP_FORCEACTIVE(
    LiteralPoolVocalPart,
    "scoring",
    "vocals",
    "slop",
    "pitch_margin",
    "phrase_value",
    "note_length_factor",
    "pitch_hit_multiplier",
    "nonpitch_hit_multiplier",
    "nonpitch_easy_multiplier",
    "vocal_cap_growth",
    "short_note_threshold_ms",
    "short_note_multiplier",
    "nonpitch_energy_threshold",
    "VocalPart.cpp",
    "i_pTalkyMatcher",
    "o_rCache.GetHitPercentage() == 0.0f",
    "noteMatched != -1",
    "=== HandlePhraseEnd singer %d ms %f\n",
    "\tNext Phrase Data:\n",
    "\tStart ms: %f\n",
    "\tEnd ms: %f\n",
    "\tBegin Note: %d\n",
    "\tEnd Note: %d\n",
    "\tEnd Of Song\n",
    "mThisPhrase->mTambourinePhrase",
    "fast song scoring should only be done on PC.",
    "InFreestyleSection()",
    "i_pA",
    "i_pB",
    "( 0.0f) <= ( fPercentage) && ( fPercentage) <= ( 1.0f)",
    "error getting min pitch for part %d at ms: %f, defaulting to 36\n",
    "error getting max pitch for part %d at ms: %f, defaulting to 84\n",
    "i_newList",
    "vector"
)
#endif
#include "game/SongDB.h"
#include "game/VocalPlayer.h"
#include "obj/Data.h"
#include "os/System.h"
#include <cfloat>

VocalPart::VocalPart(VocalPlayer *vp, int idx)
    : mPlayer(vp), mPartIndex(idx), mVocalNoteList(0), unk18(0), unk1c(0), unk20(0),
      mRemotePhraseMeterFrac(0), mPhraseScorePartMultiplier(1.0f), mPhraseScoreMax(0),
      unk3c(0), mPhraseScore(0), unk44(0), unk48(0), unk4c(0), unk50(0), unk54(0),
      unk58(0), unk84(0), unk88(-1), mSpotlightPhraseID(-1), unk98(0), unk9c(FLT_MAX),
      unka0(-FLT_MAX), unka4(0), unka8(0), mInFreestyleSection(0), unkad(0), unkb0(0),
      unkb4(0), mFirstPhraseMsToScore(0), unkbc(-1.0f), mBestSinger(0),
      mBestSingerPitchDistance(FLT_MAX), unkc8(6), mScoringEnabled(1), mPhraseRank(0) {
    SetDifficultyVariables(mPlayer->GetUser()->GetDifficulty());
}

VocalPart::~VocalPart() {}

void VocalPart::SetDifficultyVariables(int diff) {
    DataArray *voxCfg = SystemConfig("scoring", "vocals");
    mSlop = voxCfg->FindArray("slop")->Float(diff + 1);
    mPitchMaximumDistance = voxCfg->FindArray("pitch_margin")->Float(diff + 1);
    float log = std::log((double)0.1f);
    mPitchSigma = -(mPitchMaximumDistance * mPitchMaximumDistance) / log;
    mPhraseValue = voxCfg->FindArray("phrase_value")->Int(diff + 1);
    mNoteLengthFactor = voxCfg->FindArray("note_length_factor")->Float(diff + 1);
    mPitchHitMultiplier = voxCfg->FindArray("pitch_hit_multiplier")->Float(diff + 1);
    mNonPitchHitMultiplier =
        voxCfg->FindArray("nonpitch_hit_multiplier")->Float(diff + 1);
    mNonPitchEasyMultiplier = voxCfg->FindArray("nonpitch_easy_multiplier")->Float(1);
    mPhraseScoreCapGrowth = voxCfg->FindArray("vocal_cap_growth")->Float(diff + 1);
    mShortNoteThresh = voxCfg->FindFloat("short_note_threshold_ms");
    mShortNoteMult = voxCfg->FindArray("short_note_multiplier")->Float(diff + 1);
    mTalkyEnergyThreshold = voxCfg->FindFloat("nonpitch_energy_threshold");
}

void VocalPart::PostLoad() {
    mVocalNoteList = TheSongDB->GetVocalNoteList(mPartIndex);
    mFreestyleSection = mVocalNoteList->mFreestyleSections.begin();
    mVocalNoteList->CapLastFreestyleSection(TheSongDB->GetSongDurationMs());
    CalcNoteWeights();
}

void VocalPart::Start() {}
void VocalPart::StartIntro() {}

void VocalPart::UpdateSongMinMaxPitch() {
    unk9c = FLT_MAX;
    unka0 = -FLT_MAX;
    if (mVocalNoteList) {
        std::vector<VocalPhrase> &phrases = mVocalNoteList->mPhrases;
        FOREACH (it, phrases) {
            if (it->unk10 != it->unk14) {
                unk9c = Min(unk9c, it->unk24);
                unka0 = Max(unka0, it->unk28);
            }
        }
    }
}

void VocalPart::Restart(bool b1) {
    mSpotlightPhraseID = -1;
    if (!b1) {
        unkbc = -1.0f;
        unk58 = 0;
        unk3c = 0;
        unk54 = 0;
        mPhraseScore = 0;
        unk44 = 0;
        unk48 = 0;
        unk18 = 0;
        unk20 = 0;
        unk4c = 0;
        unk50 = 0;
        mInFreestyleSection = 0;
        unkad = 0;
        unkb4 = 0;
        mRemotePhraseMeterFrac = 0;
        mFirstPhraseMsToScore = 0;
        CalcNoteWeights();
        if (mVocalNoteList) {
            mThisPhrase = mVocalNoteList->mPhrases.begin();
            mPhraseScoreMax = 0;
            UpdateMinMaxPitch(mThisPhrase);
            UpdateSongMinMaxPitch();
            mFreestyleSection = mVocalNoteList->mFreestyleSections.begin();
        }
    }
}

void VocalPart::UpdateMinMaxPitch(const VocalPhrase *const &phrase) {
    const VocalPhrase *current = phrase;
    const VocalNoteList *notes = mVocalNoteList;
    const VocalPhrase *end = notes->mPhrases.end();
    if (current == end) {
        unka4 = unka8 = 0.0f;
        return;
    }
    unka4 = FLT_MAX;
    unka8 = -FLT_MAX;
    bool foundPitch = false;
    for (; current != end; ++current) {
        if (current->unk10 != current->unk14) {
            for (int i = current->unk10; i < current->unk14; ++i) {
                if (!notes->mNotes[i].IsUnpitched()) {
                    foundPitch = true;
                    unka4 = Min(unka4, current->unk24);
                    unka8 = Max(unka8, current->unk28);
                    break;
                }
            }
        }
        if (current->unk1a)
            break;
    }
    if (!foundPitch) {
        unka4 = 50.0f;
        unka8 = 67.0f;
    } else if (unka4 == unka8) {
        unka4 -= 5.0f;
        unka8 += 5.0f;
    }
}
void VocalPart::SetPaused(bool) {}
bool VocalPart::CouldScoreAgainstPart(
    float ms, TalkyMatcher *i_pTalkyMatcher, float pitch, float margin, float &targetPitch
) {
    int first = -1;
    int last = -1;
    GetNoteRange(ms, first, last);
    for (int i = first; i < last; ++i) {
        const VocalNote &note = mVocalNoteList->mNotes[i];
        if (note.IsUnpitched()) {
            MILO_ASSERT(i_pTalkyMatcher, 0x1F2);
            bool energy = i_pTalkyMatcher->mVoiceBeat.unk4 > mTalkyEnergyThreshold;
            bool voiced = i_pTalkyMatcher->mVoiceBeat.unk1;
            bool rejected = i_pTalkyMatcher->mVoiceBeat.unk0;
            if (mPlayer->IsAutoplay() || (voiced && !rejected && energy))
                return true;
        } else if (pitch != 0.0f) {
            float distance;
            float target = GetSloppyPitch(ms, i, pitch, distance);
            float difference = (float)std::fmod((double)std::fabs(target - pitch), 12.0);
            difference = Min(12.0f - difference, difference);
            if (difference < margin) {
                targetPitch = target;
                return true;
            }
        }
    }
    targetPitch = 0.0f;
    return false;
}

void VocalPart::Jump(float f1, bool) {
    unk58 = 0;
    unk3c = 0;
    unk54 = f1;
    mPhraseScore = 0;
    unk44 = 0;
    unk48 = 0;
    unk18 = 0;
    unk20 = 0;
    unk4c = 0;
    unk50 = 0;
    mInFreestyleSection = 0;
    unkad = 0;
    unkb4 = 0;
    mRemotePhraseMeterFrac = 0;
    mFirstPhraseMsToScore = 0;
    if (mVocalNoteList) {
        mThisPhrase = mVocalNoteList->mPhrases.begin();
        while (mThisPhrase != mVocalNoteList->mPhrases.end()
               && mThisPhrase->unk0 + mThisPhrase->unk4 < f1) {
            mThisPhrase++;
        }
        mFreestyleSection = mVocalNoteList->mFreestyleSections.begin();
        while (mFreestyleSection != mVocalNoteList->mFreestyleSections.end()
               && f1 > mFreestyleSection->second) {
            mFreestyleSection++;
        }
        mSpotlightPhraseID = -1;
        UpdateMinMaxPitch(mThisPhrase);
    }
}

void VocalPart::LocalDeployBandEnergy() {
    if (mInFreestyleSection)
        unkad = true;
}

void VocalPart::CalcNoteWeights() {
    mNoteWeights.clear();
    if (mVocalNoteList) {
        mNoteWeights.reserve(mVocalNoteList->mNotes.size());
        for (unsigned int i = 0; i != mVocalNoteList->mNotes.size(); ++i) {
            const VocalNote &note = mVocalNoteList->mNotes[i];
            float weight = GetNoteSliceWeight(note.mMs, note.EndMs(), i);
            mNoteWeights.push_back(weight);
        }
        mThisPhrase = mVocalNoteList->mPhrases.begin();
        mPhraseScoreMax = 0;
        unk1c = 0;
        for (const VocalPhrase *phrase = mVocalNoteList->mPhrases.begin();
             phrase != mVocalNoteList->mPhrases.end();
             ++phrase) {
            if (phrase->unk10 != phrase->unk14)
                ++unk1c;
        }
    }
}

void VocalPart::EnableScoring(bool b) { mScoringEnabled = b; }
bool VocalPart::ScoringEnabled() const { return mScoringEnabled; }

void VocalPart::ResetScoring() {
    if (!IsEmptyPhrase(mThisPhrase)) {
        mPhraseScoreMax = CalcPhraseScoreMax(mThisPhrase);
    } else
        mPhraseScoreMax = 0;
}

void VocalPart::AddScore(const VocalScoreCache &c) { AddPhrasePoints(c.unk4); }
void VocalPart::ForcePhrasePointDelta(float f1) { mPhraseScore += f1; }

void VocalPart::SetPhraseScoreMultiplier(float f1) { mPhraseScorePartMultiplier = f1; }
void VocalPart::SetPhraseRank(int i) { mPhraseRank = i; }

void VocalPart::SetRemotePhraseMeterFrac(float f1) { mRemotePhraseMeterFrac = f1; }
void VocalPart::OnGameOver() {}

int VocalPart::GetSpotlightPhrase() const { return mSpotlightPhraseID; }

void VocalPart::SetFirstPhraseMsToScore(float f1) { mFirstPhraseMsToScore = f1; }

int VocalPart::CurrentPhraseIndex() const {
    return mThisPhrase - mVocalNoteList->mPhrases.begin();
}

const VocalPhrase *VocalPart::GetFirstPhraseMarker() const {
    return mVocalNoteList->mPhrases.begin();
}

const VocalPhrase *VocalPart::GetNextPhraseMarker(const VocalPhrase *const &phrase
) const {
    if (phrase == mVocalNoteList->mPhrases.end())
        return phrase;
    return phrase + 1;
}

bool VocalPart::IsPhraseMarkerAtEnd(const VocalPhrase *const &phrase) const {
    return phrase == mVocalNoteList->mPhrases.end();
}

bool VocalNoteEndCmp(float ms, const VocalNote &note) {
    return ms < note.mMs + note.mDurationMs;
}

bool VocalPart::InEmptyPhrase() const { return IsEmptyPhrase(mThisPhrase); }

bool VocalPart::InPlayablePhrase() const { return true; }

bool VocalPart::PhraseHasUnpitchedNotes() const {
    if (mThisPhrase == mVocalNoteList->mPhrases.end())
        return false;
    return mThisPhrase->unk19;
}

bool VocalPart::InTambourinePhrase() const {
    bool result = false;
    const VocalNoteList *notes = mVocalNoteList;
    const VocalPhrase *phrase = mThisPhrase;
    if (phrase != notes->mPhrases.end() && phrase->mTambourinePhrase)
        result = true;
    return result;
}

void VocalPart::AddSingerCandidate(Singer *singer, float distance) {
    if (mBestSinger && !(distance > mBestSingerPitchDistance))
        return;
    mBestSinger = singer;
    mBestSingerPitchDistance = distance;
}

void VocalPart::ClearSingerCandidates() {
    mBestSinger = 0;
    mBestSingerPitchDistance = FLT_MAX;
}

Singer *VocalPart::GetBestSingerCandidate() { return mBestSinger; }
int VocalPart::NumPracticePhrases(const std::vector<VocalPhrase> &phrases) const {
    if (mVocalNoteList)
        return mVocalNoteList->GetNumPracticePhrases(phrases);
    return 0;
}

bool VocalPart::HasBestSingerCandidate() { return mBestSinger != 0; }

bool VocalPart::IsEmptyPhrase(const VocalPhrase *const &phrase) const {
    if (phrase == mVocalNoteList->mPhrases.end())
        return true;
    if (phrase->mTambourinePhrase)
        return false;
    if (phrase->unk10 != phrase->unk14)
        return false;
    int previous = phrase->unk10 - 1;
    if (previous >= 0) {
        const VocalNote &note = mVocalNoteList->mNotes[previous];
        if (note.mMs + note.mDurationMs > phrase->unk0)
            return false;
    }
    return true;
}

bool VocalPart::AtPhraseEnd(float ms) const {
    if (mThisPhrase != mVocalNoteList->mPhrases.end()) {
        if (ms > mThisPhrase->unk0 + mThisPhrase->unk4)
            return true;
    }
    return false;
}

float VocalPart::FramePhraseMeterFrac() const {
    bool local = !mPlayer->IsNet();
    if (local) {
        float fraction = 0.0f;
        if (fraction != mPhraseScoreMax)
            fraction = mPhraseScore / mPhraseScoreMax;
        return Clamp(0.0f, 1.0f, fraction);
    }
    return mRemotePhraseMeterFrac;
}

float VocalPart::GetFreestyleSectionDurationMs() const {
    MILO_ASSERT(InFreestyleSection(), 0x6AB);
    if (mFreestyleSection == mVocalNoteList->mFreestyleSections.end())
        return 0.0f;
    return mFreestyleSection->second - mFreestyleSection->first;
}

float VocalPart::GetOverallPartHitPercentage() const {
    if (!unk50)
        return 0.0f;
    float fPercentage = unk4c / (float)unk50;
    MILO_ASSERT(( 0.0f) <= ( fPercentage) && ( fPercentage) <= ( 1.0f), 0x6D6);
    return fPercentage;
}

int VocalPart::CalculateRemainingTambourineTicks() {
    MILO_ASSERT(mThisPhrase->mTambourinePhrase, 0x614);
    int ticks = mThisPhrase->unkc;
    const VocalPhrase *phrase = GetNextPhraseMarker(mThisPhrase);
    while (phrase != mVocalNoteList->mPhrases.end() && phrase->mTambourinePhrase) {
        ticks += phrase->unkc;
        phrase = GetNextPhraseMarker(phrase);
    }
    return ticks;
}

bool VocalPart::FramePhraseMeterFracSorter(const VocalPart *i_pA, const VocalPart *i_pB) {
    MILO_ASSERT(i_pA, 0x6C8);
    MILO_ASSERT(i_pB, 0x6C9);
    double fractionB = i_pB->FramePhraseMeterFrac();
    return i_pA->FramePhraseMeterFrac() > fractionB;
}

void VocalPart::SetVocalNoteList(VocalNoteList *i_newList) {
    MILO_ASSERT(i_newList, 0x771);
    mVocalNoteList = i_newList;
    CalcNoteWeights();
    ResetScoring();
}

float VocalPart::CalcPhraseScoreMax(const VocalPhrase *const &phrase) const {
    const VocalPhrase *current = phrase;
    const VocalNoteList *notes = mVocalNoteList;
    int begin = current->unk10;
    if (begin > 0) {
        const VocalNote &previous = notes->mNotes[begin - 1];
        if (previous.mMs + previous.mDurationMs > current->unk0)
            --begin;
    }
    int end = current->unk14;
    float total = 0.0f;
    if ((unsigned int)begin == (unsigned int)end)
        return total;
    float phraseEnd;
    float phraseStart = current->unk0;
    phraseEnd = phraseStart + current->unk4;
    for (unsigned int i = begin; i != (unsigned int)end; ++i) {
        const VocalNote &note = notes->mNotes[i];
        const float noteStart = note.mMs;
        const float duration = note.mDurationMs;
        float start = Max(noteStart, phraseStart);
        float stop = Min(noteStart + duration, phraseEnd);
        float fraction = (stop - start) / duration;
        total += fraction * mNoteWeights[i];
    }
    return total;
}

void VocalPart::Poll(float ms, const SongPos &) {
    while (mFreestyleSection != mVocalNoteList->mFreestyleSections.end()
           && ms > mFreestyleSection->second) {
        ++mFreestyleSection;
    }
    if ((mPlayer->CanDeployOverdrive() || mPlayer->mIsInCoda
         || mPlayer->IsDeployingBandEnergy())
        && (mThisPhrase == mVocalNoteList->mPhrases.end()
            || (mFreestyleSection != mVocalNoteList->mFreestyleSections.end()
                && ms >= mFreestyleSection->first && ms < mFreestyleSection->second))) {
        mInFreestyleSection = true;
    } else {
        mInFreestyleSection = false;
        unkad = false;
    }
    // Preserve the original virtual call even though its result is unused.
    mPlayer->IsNet();
    if (mPlayer->mIsInCoda && ms > unkb0)
        unkb4 = true;
    if (mInFreestyleSection)
        unk98 = 3;
    else if (mPlayer->InTambourinePhrase())
        unk98 = 2;
    else
        unk98 = 0;
    int first = -1, last = -1;
    GetNoteRange(ms, first, last);
    while (last > unk3c && unk3c < mThisPhrase->unk14) {
        unk38 += mPhraseScoreCapGrowth * mNoteWeights[unk3c];
        ++unk3c;
    }
    last = std::min(last, (int)mVocalNoteList->mNotes.size());
    bool allUnpitched = true;
    for (int i = first; i < last; ++i) {
        if (!mVocalNoteList->mNotes[i].IsUnpitched()) {
            allUnpitched = false;
            break;
        }
    }
    if (allUnpitched && first != last)
        unk98 = 1;
    VocalFrameSpewData *spew = mPlayer->mFrameSpewData;
    if (spew) {
        float targetPitch = mVocalNoteList->PitchAt(ms);
        spew->mPartData[mPartIndex].unk0 = targetPitch;
    }
}

void VocalPart::AddPhrasePoints(float points) {
    float next;
    float limit;
    float maximum;
    float previous;
    float cap;
    previous = mPhraseScore;
    maximum = mPhraseScoreMax;
    limit = unk38;
    next = previous + points;
    cap = Min(limit, maximum);
    mPhraseScore = Min(next, cap);
    float delta = mPhraseScore - previous;
    int multiplier, bonus1, bonus2;
    mPlayer->GetMultiplier(true, multiplier, bonus1, bonus2);
    unk44 += delta * (float)(bonus1 - 1);
    unk48 += delta * (float)(bonus2 - 1);
}

float VocalPart::GetBestHit(
    float ms,
    int first,
    int last,
    TalkyMatcher *i_pTalkyMatcher,
    float &pitch,
    float,
    int &octaveOffset,
    int &noteMatched,
    float &targetPitch,
    float &targetMs,
    bool &unpitched
) {
    noteMatched = -1;
    float best = 0.0f;
    const float originalPitch = pitch;
    bool voiced;
    bool rejected;
    bool bestIsUnpitched = false;
    unpitched = false;
    for (int i = first; i < last; ++i) {
        if (mVocalNoteList->mNotes[i].IsUnpitched()) {
            MILO_ASSERT(i_pTalkyMatcher, 0x46D);
            voiced = i_pTalkyMatcher->mVoiceBeat.unk1;
            rejected = i_pTalkyMatcher->mVoiceBeat.unk0;
            bool energy = i_pTalkyMatcher->mVoiceBeat.unk4 > mTalkyEnergyThreshold;
            float hit = 1.0f;
            if (mPlayer->IsAutoplay() || (voiced && !rejected && energy)) {
                if (bestIsUnpitched) {
                    MILO_ASSERT(noteMatched != -1, 0x486);
                    const VocalNote &previous = mVocalNoteList->mNotes[noteMatched];
                    const VocalNote &current = mVocalNoteList->mNotes[i];
                    float previousEnd;
                    float currentDistance;
                    float previousDistance;
                    previousEnd = previous.EndMs();
                    currentDistance = current.mMs - ms;
                    previousDistance = previousEnd - ms;
                    currentDistance = std::fabs(currentDistance);
                    previousDistance = std::fabs(previousDistance);
                    if (previousDistance < currentDistance)
                        hit = 0.0f;
                }
                if (hit >= best) {
                    pitch = originalPitch;
                    best = hit;
                    bestIsUnpitched = true;
                    noteMatched = i;
                    targetPitch = -1.0f;
                    octaveOffset = 0;
                    targetMs = ms;
                    unpitched = true;
                }
            }
        } else if (pitch != 0.0f) {
            float adjustedPitch = originalPitch;
            float noteTargetPitch;
            int adjustedOctave = octaveOffset;
            float noteTargetMs;
            float hit = ScoreNote(
                ms, i, adjustedPitch, adjustedOctave, noteTargetPitch, noteTargetMs
            );
            if (hit >= best || (hit > 0.0 && bestIsUnpitched)) {
                pitch = adjustedPitch;
                best = hit;
                bestIsUnpitched = false;
                noteMatched = i;
                targetPitch = noteTargetPitch;
                octaveOffset = adjustedOctave;
                targetMs = noteTargetMs;
                unpitched = false;
            }
        }
    }
    return best;
}

void VocalPart::CalculateScore(float ms, int noteIndex, float hit, VocalScoreCache &cache)
    const {
    if (noteIndex != -1) {
        float weight = GetNoteSliceWeight(unk54, ms, noteIndex);
        const VocalNote &note = mVocalNoteList->mNotes[noteIndex];
        float points;
        float multiplier;
        if (!note.IsUnpitched())
            multiplier = mPitchHitMultiplier;
        else if (note.mUnpitchedEasy)
            multiplier = mNonPitchHitMultiplier * mNonPitchEasyMultiplier;
        else
            multiplier = mNonPitchHitMultiplier;
        if (note.mDurationMs < mShortNoteThresh)
            multiplier *= mShortNoteMult;
        points = multiplier * (hit * weight);
        VocalFrameSpewData *spew = mPlayer->mFrameSpewData;
        if (spew) {
            spew->mPartData[mPartIndex].unk4 = points;
            spew->mPartData[mPartIndex].unk8 = unk38;
            spew->mPartData[mPartIndex].unkc = hit;
            spew->mPartData[mPartIndex].unk10 = weight;
            spew->mPartData[mPartIndex].unk14 = multiplier;
        }
        cache.unkc = points;
        if (mPhraseScore + points > unk38)
            points = unk38 - mPhraseScore;
        cache.unk4 = points;
        float available = mPhraseScore + weight * multiplier;
        float cap = Min(unk38, mPhraseScoreMax);
        float remaining = Min(available, cap) - mPhraseScore;
        MaxEq(remaining, 0.0f);
        cache.unk10 = remaining;
    }
}
bool PitchBetween(float, float, float, float &);

float VocalPart::GetSloppyPitch(float ms, int noteIndex, float pitch, float &targetMs)
    const {
    const VocalNote &note = mVocalNoteList->mNotes[noteIndex];
    float late = note.PitchAt(ms + mSlop);
    float early = note.PitchAt(ms - mSlop);
    float pitchClass = (float)std::fmod((double)pitch, 12.0);
    float lateClass = (float)std::fmod((double)late, 12.0);
    float earlyClass = (float)std::fmod((double)early, 12.0);
    float between = -1.0f;
    if (PitchBetween(pitch, late, early, between)) {
        targetMs = ms;
        return between;
    }
    float lateDistance = std::fabs(pitchClass - lateClass);
    float earlyDistance = std::fabs(pitchClass - earlyClass);
    if (lateDistance < earlyDistance) {
        targetMs = std::min(ms + mSlop, note.EndMs());
        return late;
    } else if (lateDistance > earlyDistance) {
        targetMs = std::max(ms - mSlop, note.mMs);
        return early;
    }
    bool within = false;
    if (note.mMs <= ms && ms < note.EndMs())
        within = true;
    if (within)
        targetMs = ms;
    else
        // Preserve the original argument order in this equal-distance case.
        targetMs = Clamp(ms, note.mMs, note.EndMs());
    return late;
}
void VocalPart::ScoreSinger(
    float ms,
    float pitch,
    float,
    float margin,
    int octaveOffset,
    TalkyMatcher *talkyMatcher,
    VocalScoreCache &o_rCache,
    int &resultOctave,
    float &pitchDeviation
) {
    MILO_ASSERT(o_rCache.GetHitPercentage() == 0.0f, 0x2C3);
    o_rCache.unk8 = Min(unk38, mPhraseScoreMax);
    pitchDeviation = VocalPlayer::kInvalidPitch;
    if (pitch == 0.0f && !mVocalNoteList->NoteAt(ms)) {
        o_rCache.unk0 = 1.0f;
        resultOctave = octaveOffset;
        return;
    }
    int first = -1, last = -1;
    int noteMatched;
    float targetPitch = 0.0f;
    float adjustedPitch = pitch;
    int adjustedOctave = octaveOffset;
    float targetMs;
    bool unpitched;
    GetNoteRange(ms, first, last);
    float hit = GetBestHit(
        ms,
        first,
        last,
        talkyMatcher,
        adjustedPitch,
        margin,
        adjustedOctave,
        noteMatched,
        targetPitch,
        targetMs,
        unpitched
    );
    resultOctave = adjustedOctave;
    if (noteMatched != -1) {
        pitchDeviation = std::fmod((double)(pitch - targetPitch), 12.0);
        if (pitchDeviation > 6.0f)
            pitchDeviation -= 12.0f;
        else if (pitchDeviation < -6.0f)
            pitchDeviation += 12.0f;
        if (mVocalNoteList->mNotes[noteMatched].IsUnpitched())
            unk98 = 1;
        else
            unk98 = 0;
    }
    o_rCache.unk0 = hit;
    o_rCache.unk14 = targetPitch;
    o_rCache.unk18 = targetMs;
    o_rCache.unk1c = adjustedOctave;
    o_rCache.unk20 = unpitched;
    CalculateScore(ms, noteMatched, hit, o_rCache);
}

float VocalPart::ScoreNote(
    float ms,
    int noteIndex,
    float &pitch,
    int &octaveOffset,
    float &targetPitch,
    float &targetMs
) const {
    float target = GetSloppyPitch(ms, noteIndex, pitch, targetMs);
    targetPitch = target;
    float difference = target - pitch;
    float magnitude = std::fabs(difference);
    float folded = (float)std::fmod((double)magnitude, 12.0);
    float distance = Min(12.0f - folded, folded);
    if (distance <= 2.5f) {
        int direction = difference > 0.0f ? 1 : -1;
        octaveOffset = (int)(magnitude / 12.0f + 0.5f) * direction;
        pitch += 12.0f * (float)octaveOffset;
        difference = distance;
    }
    float score = 0.0f;
    if (std::fabs(difference) <= mPitchMaximumDistance) {
        score = (float)std::exp((double)(-(difference * difference) / mPitchSigma));
        if (score < 0.01f)
            score = 0.0f;
    }
    if (GetNoteSliceWeight(unk54, ms, noteIndex) == 0.0f)
        score = 0.0f;
    return score;
}
const float kFrameTimeMs = 16.666667938232421875f;

float VocalPart::GetNoteSliceWeight(float start, float end, int index) const {
    if (end < start) {
        float temp = start;
        start = end;
        end = temp;
    }
    const VocalNote &note = mVocalNoteList->mNotes[index];
    float duration = note.mDurationMs;
    float finish = end - note.mMs;
    float position = start - note.mMs;
    float limit = 150.0f;
    if (finish < limit)
        limit = duration;
    float total = 0.0f;
    if (note.mBeginPitch == note.mEndPitch) {
        for (; position < finish;) {
            float step = std::min(kFrameTimeMs, finish - position);
            float weight;
            if (position < 0.0f)
                weight = 0.0f;
            else if (position < limit)
                weight = (float)std::pow((double)(position / limit), 0.5);
            else
                weight = 1.0f;
            total += weight * step;
            position += step;
        }
    } else {
        float baseline = 1.0f - (float)std::pow((double)(1.0f / 1.75f), 2.0);
        for (; position < finish;) {
            float step = std::min(kFrameTimeMs, finish - position);
            float weight;
            if (position < 0.0f)
                weight = 1.0f;
            else if (position > duration)
                weight = 1.0f;
            else
                weight = baseline
                    + (float
                    )std::pow((double)(2.0f * (position / duration - 0.5f) / 1.75f), 2.0);
            total += weight * step;
            position += step;
        }
    }
    return total;
}

bool PitchBetween(float pitch, float a, float b, float &adjusted) {
    float low = Min(a, b);
    float high = Max(a, b);
    while (pitch > high)
        pitch -= 12.0f;
    while (pitch < low)
        pitch += 12.0f;
    adjusted = pitch;
    if (pitch >= low && pitch <= high)
        return true;
    return false;
}

void VocalPart::Rollback(float, float ms) {
    unk58 = 0;
    unk54 = ms;
    if (mVocalNoteList) {
        mThisPhrase = mVocalNoteList->mPhrases.begin();
        while (mThisPhrase != mVocalNoteList->mPhrases.end()
               && mThisPhrase->unk0 + mThisPhrase->unk4 < ms)
            ++mThisPhrase;
        mFreestyleSection = mVocalNoteList->mFreestyleSections.begin();
        while (mFreestyleSection != mVocalNoteList->mFreestyleSections.end()
               && ms > mFreestyleSection->second)
            ++mFreestyleSection;
        mSpotlightPhraseID = -1;
        UpdateMinMaxPitch(mThisPhrase);
    }
}

bool VocalPart::NearNote(float ms) {
    int first = -1;
    int last = -1;
    GetNoteRange(ms, first, last);
    return first < last;
}

void VocalPart::AfterPoll(float ms) {
    int first, last;
    GetNoteRange(ms, first, last);
    int begin = first;
    unk54 = ms;
    unk58 = Max(begin, 0);
}

float VocalPart::GetPartHitPercentage(const std::vector<VocalPhrase> &phrases, int, int)
    const {
    if (!unk50)
        return 0.0f;
    float fPercentage = unk4c / (float)NumPracticePhrases(phrases);
    MILO_ASSERT(( 0.0f) <= ( fPercentage) && ( fPercentage) <= ( 1.0f), 0x6E4);
    return fPercentage;
}

void VocalPart::GetNoteRange(float ms, int &first, int &last) {
    float low = ms - mSlop;
    float high = ms + mSlop;
    const std::vector<VocalNote> &notes = mVocalNoteList->GetNotes();
    first = -1;
    last = -1;
    const VocalNote *note =
        std::upper_bound(notes.begin(), notes.end(), low, VocalNoteEndCmp);
    if (note != notes.end()) {
        for (; note->mMs < high && note != notes.end(); ++note) {
            int index = note - notes.begin();
            if (first == -1)
                first = index;
            last = index + 1;
        }
    }
}
