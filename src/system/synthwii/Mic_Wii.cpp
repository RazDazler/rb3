#include "Mic_Wii.h"
#include <os/Debug.h>

float MicWii::GetEarpieceVolume() const { return 0.0f; }

void MicWii::SetEarpieceVolume(float) { return; }

int MicWii::GetClipping() const { return 0; }

float MicWii::GetGain() const { return mGain; }

int MicWii::GetSampleRate() const { return 1; }

float MicWii::GetVolume() const { return mVolume; }

short *MicWii::GetBuf() { return mOutBuf; }

int MicWii::GetBufSamples() const { return mOutBufSamples; }

void MicWii::SetMicIndex(int micIndex) {
    MILO_ASSERT(micIndex >= -1 && micIndex < 4, 0x288);
    mMicIndex = micIndex;
}

bool MicWii::IsRunning() const { return mOn; }
int MicWii::GetPad() const { return mPadNum; }
void MicWii::SetPad(int pad) { mPadNum = pad; }
float MicWii::GetCompressorParam() const { return 0.0f; }
void MicWii::SetCompressorParam(float) {}
bool MicWii::GetCompressor() const { return true; }
void MicWii::SetCompressor(bool) {}
float MicWii::GetSensitivity() const { return mSensitivity; }
void MicWii::SetSensitivity(float sensitivity) { mSensitivity = sensitivity; }
float MicWii::GetOutputGain() const { return 0.0f; }
void MicWii::SetOutputGain(float) {}
bool MicWii::GetDMA() const { return false; }
void MicWii::SetDMA(bool) {}
