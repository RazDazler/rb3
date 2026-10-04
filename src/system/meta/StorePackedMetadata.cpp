#include "meta/StorePackedMetadata.h"
#include "decomp.h"

#if defined(VERSION_SZBE69_B8)
// Preserve original literal ordering while incomplete methods remain.
DECOMP_FORCEACTIVE(
    LiteralPoolStorePackedMetadata,
    "Store: file %s is missing\n",
    "Store: file %s is over budget (%d > %d)\n",
    "Store: Failed to allocated %d byte buffer for store file %s.\n",
    "Store: Failed to allocated %d byte buffer for decompressing store file %s.\n",
    "sizeof(StoreVersionHeader) == %d\n",
    "%d strings, but based on the data size, there can only be %d.\n",
    "There are %d null terminators, should have %d.\n",
    "String %d does not match up\n",
    "%sstrings",
    "%sstrings_%s",
    "%c%c%c%c",
    "sizeof(StorePackedSong) == %d\n",
    "%ssongs",
    "There are %d bytes left in song file, at %d bytes per song is %d songs, but the file says there are %d songs.\n",
    "Song %d: name %d is invalid\n",
    "Song %d: artist %d is invalid\n",
    "sizeof(StorePackedRanks) == %d\n",
    "%s(%d) : Warning: %s",
    "StorePackedMetadata.cpp",
    "!padding",
    "/preview_art/%s_nomip.png_%s",
    "/album_art/UGC_%d_keep.png_%s",
    "/preview_audio/%s_prev.bik",
    "/audio_prev/UGC_%s_prev.bik",
    "",
    "sizeof(StorePackedOfferBase) == %d\n",
    "offsetof(StorePackedOfferBase, mReview) == %d\n",
    "%soffers",
    "There are %d bytes left in offers file, at %d bytes per offer is %d offers, but the file says there are %d offers.\n",
    "sizeof(StorePackedOffer) == %d\n",
    "sizeof(StorePackedRBNOffer) == %d\n",
    "%srbn_offers",
    "There are %d bytes left in rbn_offers file, at %d bytes per offer is %d offers, but the file says there are %d offers.\n",
    "%smarquee",
    "Marquee file says is has %d entries, but at %d bytes each that would take %d bytes, and there are %d bytes left in the file.\n",
    "/title/00010000/%02x%02x%02x%02x/data/",
    "%sredemption_offers",
    "Redemption file says is has %d entries, but at %d bytes each that would take %d bytes, and there are %d bytes left in the file.\n",
    "Store: ERROR: redeemed offer %s does not match any in the store.\n",
    "sizeof(StorePackedSubmenu) == %d\n",
    "sizeof(StorePackedPage) == %d\n",
    "by_artist",
    "by_song_first_letter",
    "by_subgenre",
    "by_year_released",
    "by_author",
    "by_label",
    "by_difficulty",
    "by_review",
    "by_release_date",
    "by_pack_first_letter",
    "%spages",
    "store",
    "title_starting_indices",
    "local_metadata",
    "PCPW",
    "%sversion",
    "Store: delete index by popping tmpcache failed: %d\n",
    "Store: delete index by EC_DeleteContents failed: %d\n",
    "Store: offer %s from ecommerce says titleid %llx but store index data says %llx\n",
    "Store: offer %s from ecommerce says there are %d contents but store index data says %d\n",
    "MaxUserFileSize",
    "Store: for offer %s, ecommerce lists content units [",
    "%d, ",
    "%d] but store index lists [",
    "%d]\n",
    "Store: ECContentCatalogInfo %d has null licensePricings",
    "offer_id",
    "Store: upgrade offer %s says upgrade is at %llx %d, but store index data says %llx %d",
    "CNTSDGetUserAvailableArea(%d, %d, %d, %d);\n",
    "async op %d done: %d err: %d\n",
    "DebugPurchase: out of money\n",
    "DebugPurchase: %s\n",
    "DebugDownload: found offer %s\n",
    "DebugDownload: EC_DownloadTitle %llx\n",
    "DebugDownload: downloading %d content units\n",
    "DebugDownload: failed, so quitting.\n",
    "DebugDownload: starting a content refresh.\n",
    "%s(%d): %s unhandled msg: %s",
    "vector"
)
#endif
#include "meta/StoreOffer.h"
#include "os/CommerceMgr_Wii.h"

StoreMetadataManager TheStoreMetadata;
std::vector<int> StoreMetadataManager::mSetlistOffers;
bool gDebugMakeAllSongsAvailable;
bool gDebugDontRelyOnCommerceServer;

bool StoreMetadataManager::LoadingFailed() const { return mLoadingState == 11; }
StoreError StoreMetadataManager::LoadError() const { return (StoreError)mErrorMsg; }
void StoreMetadataManager::SetMetadataIndex(
    unsigned long long title, unsigned short content, long size
) {
    mMetadataTitleId = title;
    unk90 = content;
    unk94 = size;
}
unsigned long long StorePackedSong::DataTitle() const {
    return WiiCommerceMgr::MakeDataTitleId(&unk6);
}

bool StoreLoadPackedFile(const char *, bool, int, bool, bool, char **, char **, char **, int *) {

}

bool StoreStringTable::Load(const char *cc) {
    char buf[256];
    sprintf(buf, "%sstrings", cc);
    bool nonloc = mNonLocalized.LoadFile(buf);
    sprintf(buf, "%sstrings_%s", cc, SystemLanguage().mStr);
    return nonloc && mLocalized.LoadFile(buf);
}

bool StoreStringTable::IsValid(int i) {
    if (i & 0x8000U) {
        if (mLocalized.mNumStrings <= (i & 0x7FFFU))
            return false;
        else
            return true;
    } else
        return i >= 0 && i < mNonLocalized.mNumStrings;
}

const char *StorePackedSong::GetShortName() const {
    return TheStoreMetadata.GetString(unk4);
}

const char *StorePackedSong::GetName() const {
    return TheStoreMetadata.GetString(mNameIndex);
}

const char *StorePackedSong::GetArtist() const {
    return TheStoreMetadata.GetString(mArtistIndex);
}

const char *StorePackedSong::GetDataTitle() const {
    return MakeString("%c%c%c%c", unk6, unk7, unk8, unk9);
}

const char *StorePackedSong::GetUpgradeDataTitle() const {
    return MakeString("%c%c%c%c", unkc, unkd, unke, unkf);
}

StoreSongTable::~StoreSongTable() {
    if (mBuffer)
        _MemFree(mBuffer);
}

bool StoreSongTable::Load(const char *cc) {
    char buf[256];
    sprintf(buf, "%ssongs", cc);

    char *loc12c;
    char *loc130;
    bool ret = StoreLoadPackedFile(
        buf, true, 0x40000, false, true, &mBuffer, &loc12c, &loc130, &mNumSongs
    );
    if (!ret)
        return ret;
    else {
        loc12c += mNumSongs;
        int diff = loc130 - loc12c;
        int u1 = diff / 0x1cul;
        if (u1 != mNumSongs) {
            MILO_LOG(
                "There are %d bytes left in song file, at %d bytes per song is %d songs, but the file says there are %d songs.\n",
                diff,
                0x1cul,
                u1,
                mNumSongs
            );
        }
        mSongs = (StorePackedSong *)loc12c;
        for (int i = 0; i < mNumSongs; i++) {
            StorePackedSong *song = &mSongs[i];
            song->EndianFix();
            if (!TheStoreMetadata.mStringTable->IsValid(song->mNameIndex)) {
                MILO_LOG("Song %d: name %d is invalid\n", i, song->mNameIndex);
            }
            if (!TheStoreMetadata.mStringTable->IsValid(song->mArtistIndex)) {
                MILO_LOG("Song %d: artist %d is invalid\n", i, song->mArtistIndex);
            }
        }
        return true;
    }
}

String StorePackedOfferBase::GetOfferId() const {
    String ret;
    ret.reserve(0x11);
    memcpy((void *)ret.c_str(), mId, 16);
    char *ptr = (char *)ret.c_str();
    ptr[16] = 0;
    return ret;
}

String StorePackedOfferBase::GetUpgradeId() const {
    String ret;
    ret.reserve(0x11);
    memcpy((void *)ret.c_str(), mUpgradeId, 16);
    char *ptr = (char *)ret.c_str();
    ptr[16] = 0;
    return ret;
}

const char *StorePackedOfferBase::GetName() const {
    return TheStoreMetadata.GetString(mNameIndex);
}

StoreOfferTable::~StoreOfferTable() {
    if (mBuffer) {
        _MemFree(mBuffer);
        mBuffer = 0;
    }
    delete[] mBufferNewRelease;
    mBufferNewRelease = 0;
}

#define BYTES_PER_OFFER 69UL

struct test {
    StorePackedOffer **n;
    StorePackedOffer **b[4];
};

bool StoreOfferTable::Load(const char *cc) {
    char buf[256];
    sprintf(buf, "%soffers", cc);
    struct test t;
    StorePackedOffer **loc130;
    bool ret = StoreLoadPackedFile(
        buf,
        true,
        0x40000,
        true,
        true,
        &mBuffer,
        (char **)&t.n,
        (char **)&loc130,
        &mNumOffers
    );
    if (!ret)
        return ret;
    else {
        int diff = (int)loc130 - (int)t.n;
        int actualNumOffers = diff / BYTES_PER_OFFER;
        if (actualNumOffers < mNumOffers) {
            MILO_LOG(
                "There are %d bytes left in offers file, at %d bytes per offer is %d offers, but the file says there are %d offers.\n",
                diff,
                BYTES_PER_OFFER,
                actualNumOffers,
                mNumOffers
            );
        }
        mOffers = t.n;
        void *buf = new StoreOfferState[mNumOffers];
        mBufferNewRelease = (StoreOfferState *)buf;
        memset(buf, 0, mNumOffers * sizeof(StoreOfferState));

        t.n += mNumOffers;
        t.b[0] = 0;
        t.b[1] = 0;
        t.b[2] = 0;
        for (int i = 0; i < mNumOffers; i++) {
            StorePackedOffer *curOffer = mOffers[i];
            curOffer->EndianFix();
            (((char **)&t.b[0])[curOffer->OfferType()])++;
        }
        return true;
    }
}

int StoreOfferTable::OfferIndex(const StorePackedOfferBase *base) const {
    for (int i = 0; i < mNumOffers; i++) {
        if (base == mOffers[i])
            return i;
    }
    return -1;
}

StoreRbnOfferTable::~StoreRbnOfferTable() {
    if (mBuffer) {
        _MemFree(mBuffer);
        mBuffer = 0;
    }
    delete[] mBufferNewRelease;
    mBufferNewRelease = 0;
}

bool StoreRbnOfferTable::Load(const char *cc) {}

int StoreRbnOfferTable::OfferIndex(const StorePackedOfferBase *base) const {
    for (int i = 0; i < mNumOffers; i++) {
        if (base == mOffers[i])
            return i;
    }
    return -1;
}

Symbol StorePackedPage::DefaultSort() const {
    switch (mDefaultSort) {
    case 1:
        return "by_artist";
    case 2:
        return "by_song_first_letter";
    case 3:
        return "by_subgenre";
    case 4:
        return "by_year_released";
    case 5:
        return "by_author";
    case 6:
        return "by_label";
    case 7:
        return "by_difficulty";
    case 8:
        return "by_review";
    case 9:
        return "by_release_date";
    case 10:
        return "by_pack_first_letter";
    default:
        return "by_artist";
    }
}

void StorePage::LoadFromBuffer(char *buffer, unsigned short num) {
    mPageNumber = num;
    mPage = (StorePackedPage *)buffer;
    mPage->EndianFix();
}

StorePackedOffer *StorePage::Offer(int idx) const {
    if (mPage->mHasOffers) {
        int key = mOffers[idx];
        if (!(key & 0x8000))
            return TheStoreMetadata.mOfferTable->mOffers[key];
    }
    return nullptr;
}

StorePage *StorePageTable::GetPage(unsigned short idx) {
    std::map<unsigned short, StorePage *>::const_iterator it = mPageLookup.find(idx);
    if (it != mPageLookup.end())
        return it->second;
    else
        return nullptr;
}

void StoreMetadataManager::Init() {
    SetName("store", ObjectDir::sMainDir);
    mLoadingState = 0;
    mContentSize = 0;
    mErrorMsg = 0;
    if (SystemConfig("store", "local_metadata")->Int(1)) {
        gDebugMakeAllSongsAvailable = true;
        gDebugDontRelyOnCommerceServer = true;
        mFlags |= 1;
    }
}

void StoreOfferState::UpdateFlags(
    StorePackedOfferBase *base, unsigned char c1, unsigned char c2
) {
    mFlags &= 0xC0;
    if (c1 & 2)
        mFlags |= 1;
    if (c1 & 1)
        mFlags |= 2;
    if (c1 & 8)
        mFlags |= 0x20;
    if (c1 & 0x10)
        mFlags |= 0x10;
    if (c2 & 2)
        mFlags |= 8;
}

void StoreMetadataManager::UpdateOfferOwnership() {
    for (int i = 0; i < mOfferTable->mNumOffers; i++) {
    }
}

StorePage *StoreMetadataManager::LoadPage(unsigned short idx) {
    mCurrentPage = mPageTable->GetPage(idx);
    return mCurrentPage;
}

void StoreMetadataManager::Load(const char *cc) {
    if (!(mFlags & 0xC)) {
        mBasePath = cc;
        mErrorMsg = 0;
        SetLoadingState(1);
        mFlags |= 4;
    }
}

void StoreMetadataManager::Unload() {
    RELEASE(mStringTable);
    RELEASE(mOfferTable);
    RELEASE(mRbnOfferTable);
}

StoreOfferState *StoreMetadataManager::GetOfferStatus(const StorePackedOfferBase *base) {
    int idx = mOfferTable->OfferIndex(base);
    if (idx >= 0) {
        return &mOfferTable->mBufferNewRelease[idx];
    } else {
        idx = mRbnOfferTable->OfferIndex(base);
        if (idx >= 0)
            return &mRbnOfferTable->mBufferNewRelease[idx];
    }
    return 0;
}
