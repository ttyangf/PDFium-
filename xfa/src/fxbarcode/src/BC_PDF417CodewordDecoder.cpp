// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "barcode.h"
#include "include/BC_PDF417Common.h"
#include "include/BC_PDF417CodewordDecoder.h"
#define     SYMBOL_TABLE_Length       2787
#define     Float_MAX_VALUE           2147483647
FX_FLOAT CBC_PDF417CodewordDecoder::RATIOS_TABLE[2787][8] = {0};
CBC_PDF417CodewordDecoder::CBC_PDF417CodewordDecoder()
{
}
CBC_PDF417CodewordDecoder::~CBC_PDF417CodewordDecoder()
{
}
void CBC_PDF417CodewordDecoder::Initialize()
{
    for (FX_INT32 i = 0; i < SYMBOL_TABLE_Length; i++) {
        FX_INT32 currentSymbol = CBC_PDF417Common::SYMBOL_TABLE[i];
        FX_INT32 currentBit = currentSymbol & 0x1;
        for (FX_INT32 j = 0; j < CBC_PDF417Common::BARS_IN_MODULE; j++) {
            FX_FLOAT size = 0.0f;
            while ((currentSymbol & 0x1) == currentBit) {
                size += 1.0f;
                currentSymbol >>= 1;
            }
            currentBit = currentSymbol & 0x1;
            RATIOS_TABLE[i][CBC_PDF417Common::BARS_IN_MODULE - j - 1] = size / CBC_PDF417Common::MODULES_IN_CODEWORD;
        }
    }
}
void CBC_PDF417CodewordDecoder::Finalize()
{
}
FX_INT32 CBC_PDF417CodewordDecoder::getDecodedValue(CFX_Int32Array& moduleBitCount)
{
    CFX_Int32Array* array = sampleBitCounts(moduleBitCount);
    FX_INT32 decodedValue = getDecodedCodewordValue(*array);
    delete array;
    if (decodedValue != -1) {
        return decodedValue;
    }
    return getClosestDecodedValue(moduleBitCount);
}
CFX_Int32Array* CBC_PDF417CodewordDecoder::sampleBitCounts(CFX_Int32Array& moduleBitCount)
{
    FX_FLOAT bitCountSum = (FX_FLOAT)CBC_PDF417Common::getBitCountSum(moduleBitCount);
    CFX_Int32Array* bitCount = FX_NEW CFX_Int32Array();
    bitCount->SetSize(CBC_PDF417Common::BARS_IN_MODULE);
    FX_INT32 bitCountIndex = 0;
    FX_INT32 sumPreviousBits = 0;
    for (FX_INT32 i = 0; i < CBC_PDF417Common::MODULES_IN_CODEWORD; i++) {
        FX_FLOAT sampleIndex = bitCountSum / (2 * CBC_PDF417Common::MODULES_IN_CODEWORD) + (i * bitCountSum) / CBC_PDF417Common::MODULES_IN_CODEWORD;
        if (sumPreviousBits + moduleBitCount.GetAt(bitCountIndex) <= sampleIndex) {
            sumPreviousBits += moduleBitCount.GetAt(bitCountIndex);
            bitCountIndex++;
        }
        bitCount->SetAt(bitCountIndex, bitCount->GetAt(bitCountIndex) + 1);
    }
    return bitCount;
}
FX_INT32 CBC_PDF417CodewordDecoder::getDecodedCodewordValue(CFX_Int32Array& moduleBitCount)
{
    FX_INT32 decodedValue = getBitValue(moduleBitCount);
    return CBC_PDF417Common::getCodeword(decodedValue) == -1 ? -1 : decodedValue;
}
FX_INT32 CBC_PDF417CodewordDecoder::getBitValue(CFX_Int32Array& moduleBitCount)
{
    FX_INT64 result = 0;
    for (FX_INT32 i = 0; i < moduleBitCount.GetSize(); i++) {
        for (FX_INT32 bit = 0; bit < moduleBitCount.GetAt(i); bit++) {
            result = (result << 1) | (i % 2 == 0 ? 1 : 0);
        }
    }
    return (FX_INT32) result;
}
FX_INT32 CBC_PDF417CodewordDecoder::getClosestDecodedValue(CFX_Int32Array& moduleBitCount)
{
    FX_INT32 bitCountSum = CBC_PDF417Common::getBitCountSum(moduleBitCount);
    CFX_FloatArray bitCountRatios;
    bitCountRatios.SetSize(CBC_PDF417Common::BARS_IN_MODULE);
    for (FX_INT32 i = 0; i < bitCountRatios.GetSize(); i++) {
        bitCountRatios[i] = moduleBitCount.GetAt(i) / (FX_FLOAT) bitCountSum;
    }
    FX_FLOAT bestMatchError = (FX_FLOAT)Float_MAX_VALUE;
    FX_INT32 bestMatch = -1;
    for (FX_INT32 j = 0; j < SYMBOL_TABLE_Length; j++) {
        FX_FLOAT error = 0.0f;
        for (FX_INT32 k = 0; k < CBC_PDF417Common::BARS_IN_MODULE; k++) {
            FX_FLOAT diff = RATIOS_TABLE[j][k] - bitCountRatios[k];
            error += diff * diff;
        }
        if (error < bestMatchError) {
            bestMatchError = error;
            bestMatch = CBC_PDF417Common::SYMBOL_TABLE[j];
        }
    }
    return bestMatch;
}