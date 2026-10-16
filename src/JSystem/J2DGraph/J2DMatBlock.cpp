#include "JSystem/JUtility/JUTTexture.hpp"

JUTTexture* J2DMatBlockDummy() {
    return new (JKRHeapToken::Dummy) JUTTexture;
}
