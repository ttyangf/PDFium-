// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "barcode.h"
#include "include/BC_Encoder.h"
#include "include/BC_CommonBitMatrix.h"
#include "include/BC_Dimension.h"
#include "include/BC_SymbolShapeHint.h"
#include "include/BC_SymbolInfo.h"
#include "include/BC_EncoderContext.h"
#include "include/BC_HighLevelEncoder.h"
#include "include/BC_C40Encoder.h"
#include "include/BC_X12Encoder.h"
CBC_X12Encoder::CBC_X12Encoder()
{
}
CBC_X12Encoder::~CBC_X12Encoder()
{
}
FX_INT32 CBC_X12Encoder::getEncodingMode()
{
    return X12_ENCODATION;
}
void CBC_X12Encoder::Encode(CBC_EncoderContext &context, FX_INT32 &e)
{
    CFX_WideString buffer;
    while (context.hasMoreCharacters()) {
        FX_WCHAR c = context.getCurrentChar();
        context.m_pos++;
        encodeChar(c, buffer, e);
        if (e != BCExceptionNO) {
            return;
        }
        FX_INT32 count = buffer.GetLength();
        if ((count % 3) == 0) {
            writeNextTriplet(context, buffer);
            FX_INT32 newMode = CBC_HighLevelEncoder::lookAheadTest(context.m_msg, context.m_pos, getEncodingMode());
            if (newMode != getEncodingMode()) {
                context.signalEncoderChange(newMode);
                break;
            }
        }
    }
    handleEOD(context, buffer, e);
}
void CBC_X12Encoder::handleEOD(CBC_EncoderContext &context, CFX_WideString &buffer, FX_INT32 &e)
{
    context.updateSymbolInfo(e);
    if (e != BCExceptionNO) {
        return;
    }
    FX_INT32 available = context.m_symbolInfo->m_dataCapacity - context.getCodewordCount();
    FX_INT32 count = buffer.GetLength();
    if (count == 2) {
        context.writeCodeword(CBC_HighLevelEncoder::X12_UNLATCH);
        context.m_pos -= 2;
        context.signalEncoderChange(ASCII_ENCODATION);
    } else if (count == 1) {
        context.m_pos--;
        if (available > 1) {
            context.writeCodeword(CBC_HighLevelEncoder::X12_UNLATCH);
        }
        context.signalEncoderChange(ASCII_ENCODATION);
    }
}
FX_INT32 CBC_X12Encoder::encodeChar(FX_WCHAR c, CFX_WideString &sb, FX_INT32 &e)
{
    if (c == '\r') {
        sb += (FX_WCHAR)'\0';
    } else if (c == '*') {
        sb += (FX_WCHAR)'\1';
    } else if (c == '>') {
        sb += (FX_WCHAR)'\2';
    } else if (c == ' ') {
        sb += (FX_WCHAR)'\3';
    } else if (c >= '0' && c <= '9') {
        sb += (FX_WCHAR) (c - 48 + 4);
    } else if (c >= 'A' && c <= 'Z') {
        sb += (FX_WCHAR) (c - 65 + 14);
    } else {
        CBC_HighLevelEncoder::illegalCharacter(c, e);
        BC_EXCEPTION_CHECK_ReturnValue(e, -1);
    }
    return 1;
}