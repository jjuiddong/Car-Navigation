
#include "stdafx.h"
#include "cntzimg.h"

using namespace gis2;


cCntZImg::cCntZImg()
	: m_type(Type::CNT_Z)
	, m_data(nullptr)
	, m_width(0)
	, m_height(0)
	, m_bDecoderCanIgnoreMask(false)
{
}

cCntZImg::~cCntZImg()
{
	Clear();
}


// uncompress cntzimg data
// return uncompress buffer, fail? return nullptr
float* cCntZImg::UnCompress(const string &fileName)
{
	const int fileSize = (int)StrPath(fileName).FileSize();
	if (fileSize < 100)
		return nullptr;
	std::ifstream ifs(fileName, std::ios::binary);
	if (!ifs.is_open())
		return nullptr;

	BYTE *lercBlob = new BYTE[(unsigned int)fileSize];
	ifs.read((char*)lercBlob, fileSize);
	uint w = 0, h = 0;
	float *zImg3 = nullptr;
	bool res = false;

	{
		char format[11] = { 0, };
		strncpy(format, (char*)lercBlob, 10);
		if (string("CntZImage ") != format)
			goto $error;

		int *src = (int*)(lercBlob + 10);
		const int version = *src++;
		const int type = *src++;
		const int width = *src++;
		const int height = *src++;
		if ((version != 11) || (type != 8))
			goto $error;
		w = (uint)width;
		h = (uint)height;
	}

	if (w * h > 10000000)
		goto $error; // too large

	zImg3 = new float[w * h];
	memset(zImg3, 0, w * h * sizeof(float));
	res = UnCompress(lercBlob, fileSize, nullptr, 1, w, h, 1
		, (uint)DataType::DT_Float, (void*)zImg3);
	if (!res)
		goto $error;

	SAFE_DELETEA(lercBlob);
	return zImg3;

$error:
	SAFE_DELETEA(lercBlob);
	SAFE_DELETEA(zImg3);
	return nullptr;
}


// decode lerc data
bool cCntZImg::UnCompress(const BYTE* pLercBlob, uint blobSize
	, BYTE* pValidBytes, uint nDim, uint nCols, uint nRows
	, int nBands, uint dataType, void* pData)
{
	if (!pLercBlob || !blobSize || !pData 
		|| dataType >= (uint)DataType::DT_Undefined 
		|| nDim <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0)
		return false;

	return DecodeTempl((float*)pData, pLercBlob, blobSize, nDim, nCols, nRows, nBands);
}


// decode
bool cCntZImg::DecodeTempl(float *pData, const BYTE* pLercBlob
	, uint numBytesBlob, uint nDim, uint nCols, uint nRows, int nBands)
{
	if (!pData || nDim <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0 
		|| !pLercBlob || !numBytesBlob)
		return false;

	const BYTE *pByte = pLercBlob;
	BYTE *pByte1 = (BYTE*)pLercBlob;

	uint numBytesHeaderBand0 = ComputeNumBytesNeededToReadHeader(false);
	uint numBytesHeaderBand1 = ComputeNumBytesNeededToReadHeader(true);

	for (int iBand = 0; iBand < nBands; iBand++)
	{
		uint numBytesHeader = iBand == 0 ? numBytesHeaderBand0 : numBytesHeaderBand1;
		if ((size_t)(pByte - pLercBlob) + numBytesHeader > numBytesBlob)
			return false;

		const bool onlyZPart = iBand > 0;
		if (!ReadCntZImg(&pByte1, 1e12, false, onlyZPart))
			return false;

		if (m_width != nCols || m_height != nRows)
			return false;

		float* arr = pData + nCols * nRows * iBand;
		if (!Convert(arr))
			return false;
	}

	return true;
}


bool cCntZImg::Convert(float* arr)
{
	if (!arr || !GetSize())
		return false;

	const bool fltPnt = (typeid(*arr) == typeid(double)) || (typeid(*arr) == typeid(float));

	const sCntZ* srcPtr = GetData();
	float* dstPtr = arr;
	const uint num = m_width * m_height;
	for (uint k = 0; k < num; k++)
	{
		if (srcPtr->cnt > 0)
			*dstPtr = fltPnt ? (float)srcPtr->z : (float)floor(srcPtr->z + 0.5);
		srcPtr++;
		dstPtr++;
	}
	return true;
}


bool cCntZImg::ReadCntZImg(BYTE** ppByte, double maxZError
	, bool onlyHeader, bool onlyZPart)
{
	if (!ppByte || !*ppByte)
		return false;

	size_t len = GetTypeString().length();
	string typeStr(len, '0');
	memcpy(&typeStr[0], *ppByte, len);
	*ppByte += len;

	if (typeStr != GetTypeString())
		return false;

	int version = 0, type = 0;
	uint width = 0, height = 0;
	double maxZErrorInFile = 0;

	BYTE* ptr = *ppByte;

	memcpy(&version, ptr, sizeof(int));  ptr += sizeof(int);
	memcpy(&type, ptr, sizeof(int));  ptr += sizeof(int);
	memcpy(&height, ptr, sizeof(uint));  ptr += sizeof(uint);
	memcpy(&width, ptr, sizeof(uint));  ptr += sizeof(uint);
	memcpy(&maxZErrorInFile, ptr, sizeof(double));  ptr += sizeof(double);

	*ppByte = ptr;

	if (version != 11 || type != (int)m_type)
		return false;

	if (width > 20000 || height > 20000)
		return false;

	if (maxZErrorInFile > maxZError)
		return false;

	if (onlyHeader)
		return true;

	if (!onlyZPart && !ResizeFill0(width, height)) // init with (0,0), used below
		return false;

	m_bDecoderCanIgnoreMask = false;

	for (int iPart = 0; iPart < 2; iPart++)
	{
		const bool zPart = iPart ? true : false;    // first cnt part, then z part

		if (!zPart && onlyZPart)
			continue;

		uint numTilesVert = 0, numTilesHori = 0, numBytes = 0;
		float maxValInImg = 0;

		BYTE* ptr = *ppByte;

		memcpy(&numTilesVert, ptr, sizeof(uint));  ptr += sizeof(uint);
		memcpy(&numTilesHori, ptr, sizeof(uint));  ptr += sizeof(uint);
		memcpy(&numBytes, ptr, sizeof(uint));  ptr += sizeof(uint);
		memcpy(&maxValInImg, ptr, sizeof(float));  ptr += sizeof(float);

		*ppByte = ptr;
		BYTE* bArr = ptr;

		if (!zPart && numTilesVert == 0 && numTilesHori == 0)    // no tiling for this cnt part
		{
			if (numBytes == 0)    // cnt part is const
			{
				sCntZ *dstPtr = GetData();
				for (uint i = 0; i < m_height; i++)
					for (uint j = 0; j < m_width; j++)
					{
						dstPtr->cnt = maxValInImg;
						dstPtr++;
					}

				if (maxValInImg > 0)
					m_bDecoderCanIgnoreMask = true;
			}

			if (numBytes > 0) // cnt part is binary mask, use fast RLE class
			{
				assert(0);
			//	// decompress to bit mask
			//	BitMask bitMask(width_, height_);
			//	RLE rle;
			//	if (!rle.decompress(bArr, width_ * height_ * 2, (Byte*)bitMask.Bits(), bitMask.Size()))
			//		return false;

			//	CntZ* dstPtr = getData();
			//	for (int k = 0, i = 0; i < height_; i++)
			//		for (int j = 0; j < width_; j++, k++, dstPtr++)
			//			dstPtr->cnt = bitMask.IsValid(k) ? 1.0f : 0.0f;
			}
		}
		else if (!ReadTiles(zPart, maxZErrorInFile, numTilesVert, numTilesHori
			, maxValInImg, bArr))
			return false;

		*ppByte += numBytes;
	}

	m_tmpDataVec.clear();
	return true;
}


bool cCntZImg::ReadTiles(bool zPart, double maxZErrorInFile, uint numTilesVert
	, uint numTilesHori, float maxValInImg, BYTE* bArr)
{
	BYTE* ptr = bArr;

	for (uint iTile = 0; iTile <= numTilesVert; iTile++)
	{
		uint tileH = m_height / numTilesVert;
		uint i0 = iTile * tileH;
		if (iTile == numTilesVert)
			tileH = m_height % numTilesVert;

		if (tileH == 0)
			continue;

		for (uint jTile = 0; jTile <= numTilesHori; jTile++)
		{
			uint tileW = m_width / numTilesHori;
			uint j0 = jTile * tileW;
			if (jTile == numTilesHori)
				tileW = m_width % numTilesHori;

			if (tileW == 0)
				continue;

			if (iTile == 3 && jTile == 5) {
				int a = 0;
			}

			bool rv = false;
			if (zPart)
				rv = ReadZTile(&ptr, i0, i0 + tileH, j0, j0 + tileW
					, maxZErrorInFile, maxValInImg);
			if (!rv)
				return false;
		}
	}

	return true;
}


bool cCntZImg::ReadZTile(BYTE** ppByte, uint i0, uint i1, uint j0, uint j1
	, double maxZErrorInFile, float maxZInImg)
{
	BYTE* ptr = *ppByte;
	int numPixel = 0;

	BYTE comprFlag = *ptr++;
	int bits67 = comprFlag >> 6;
	comprFlag &= 63;

	if (comprFlag == 2) // entire zTile is constant 0 (if valid or invalid doesn't matter)
	{
		for (uint i = i0; i < i1; i++)
		{
			sCntZ* dstPtr = GetData() + i * m_width + j0;
			for (uint j = j0; j < j1; j++)
			{
				if (dstPtr->cnt > 0)
					dstPtr->z = 0;
				dstPtr++;
			}
		}

		*ppByte = ptr;
		return true;
	}

	if (comprFlag > 3)
		return false;

	if (comprFlag == 0)
	{
		// read z's as flt arr uncompressed
		const float* srcPtr = (const float*)ptr;

		for (uint i = i0; i < i1; i++)
		{
			sCntZ* dstPtr = GetData() + i * m_width + j0;
			for (uint j = j0; j < j1; j++)
			{
				if (dstPtr->cnt > 0)
				{
					dstPtr->z = *srcPtr++;
					numPixel++;
				}
				dstPtr++;
			}
		}

		ptr += numPixel * sizeof(float);
	}
	else
	{
		// read z's as int arr bit stuffed
		int n = (bits67 == 0) ? 4 : 3 - bits67;
		float offset = 0;
		if (!ReadFlt(&ptr, offset, (uint)n))
			return false;

		if (comprFlag == 3)
		{
			for (uint i = i0; i < i1; i++)
			{
				sCntZ* dstPtr = GetData() + i * m_width + j0;
				for (uint j = j0; j < j1; j++)
				{
					if (dstPtr->cnt > 0)
						dstPtr->z = offset;
					dstPtr++;
				}
			}
		}
		else
		{
			vector<unsigned int>& dataVec = m_tmpDataVec;
			cBitStuffer bitStuffer;
			if (!bitStuffer.Read(&ptr, dataVec))
				return false;

			double invScale = 2 * maxZErrorInFile;
			unsigned int* srcPtr = &dataVec[0];

			if (m_bDecoderCanIgnoreMask)
			{
				for (uint i = i0; i < i1; i++)
				{
					sCntZ* dstPtr = GetData() + i * m_width + j0;
					for (uint j = j0; j < j1; j++)
					{
						float z = (float)(offset + *srcPtr++ * invScale);
						dstPtr->z = std::min(z, maxZInImg);    // make sure we stay in the orig range
						dstPtr++;
					}
				}
			}
			else
			{
				assert(0);
	//			for (int i = i0; i < i1; i++)
	//			{
	//				CntZ* dstPtr = getData() + i * width_ + j0;
	//				for (int j = j0; j < j1; j++)
	//				{
	//					if (dstPtr->cnt > 0)
	//					{
	//						float z = (float)(offset + *srcPtr++ * invScale);
	//						dstPtr->z = std::min(z, maxZInImg);    // make sure we stay in the orig range
	//					}
	//					dstPtr++;
	//				}
	//			}
			}
		}
	}

	*ppByte = ptr;
	return true;
}


bool cCntZImg::ReadFlt(BYTE** ppByte, float& z, uint numBytes)
{
	BYTE* ptr = *ppByte;

	if (numBytes == 1)
	{
		char c = *((char*)ptr);
		z = c;
	}
	else if (numBytes == 2)
	{
		short s;
		memcpy(&s, ptr, sizeof(short));
		z = s;
	}
	else if (numBytes == 4)
	{
		memcpy(&z, ptr, sizeof(float));
	}
	else
		return false;

	*ppByte = ptr + numBytes;
	return true;
}


bool cCntZImg::ResizeFill0(uint width, uint height)
{
	if (!Resize(width, height))
		return false;
	memset(GetData(), 0, width * height * sizeof(float));
	return true;
}


bool cCntZImg::Resize(uint width, uint height)
{
	if (width <= 0 || height <= 0)
		return false;

	if (width == m_width && height == m_height && m_data)
		return true;

	SAFE_DELETEA(m_data);
	m_width = 0;
	m_height = 0;

	m_data = new sCntZ[width * height];
	if (!m_data)
		return false;

	m_width = width;
	m_height = height;
	return true;
}


uint cCntZImg::ComputeNumBytesNeededToReadHeader(bool onlyZPart)
{
	uint cnt = (unsigned int)GetTypeString().length();  // "CntZImage ", 10 bytes
	cnt += 4 * sizeof(int);       // version, type, width, height
	cnt += 1 * sizeof(double);    // maxZError
	if (!onlyZPart)
		cnt += 3 * sizeof(int) + sizeof(float);    // cnt part
	cnt += 3 * sizeof(int) + sizeof(float);    // z part
	cnt += 1;
	return cnt;
}


void cCntZImg::Clear()
{
	SAFE_DELETEA(m_data);
}



//----------------------------------------------------------------------------------
// cBitStuffer
bool cBitStuffer::Read(BYTE** ppByte, vector<uint>& dataVec)
{
	if (!ppByte)
		return false;

	BYTE numBitsByte = **ppByte;
	(*ppByte)++;

	int bits67 = numBitsByte >> 6;
	int n = (bits67 == 0) ? 4 : 3 - bits67;

	numBitsByte &= 63;    // bits 0-5;

	uint numElements = 0;
	if (!ReadUInt(ppByte, numElements, n))
		return false;

	if (numBitsByte >= 32)
		return false;

	int numBits = numBitsByte;
	uint numUInts = (numElements * numBits + 31) / 32;
	dataVec.resize(numElements, 0);    // init with 0

	if (numUInts > 0)    // numBits can be 0
	{
		uint numBytes = numUInts * sizeof(uint);
		uint* arr = (uint*)(*ppByte);

		uint* srcPtr = arr;
		for (uint i = 0; i < numUInts; i++)
		{
			srcPtr++;
		}

		// needed to save the 0-3 bytes not used in the last UInt
		srcPtr--;
		uint lastUInt;
		memcpy(&lastUInt, srcPtr, sizeof(uint));
		uint numBytesNotNeeded = NumTailBytesNotNeeded(numElements, numBits);
		uint n = numBytesNotNeeded;
		while (n--)
		{
			uint val;
			memcpy(&val, srcPtr, sizeof(uint));
			val <<= 8;
			memcpy(srcPtr, &val, sizeof(uint));
		}

		// do the un-stuffing
		srcPtr = arr;
		uint* dstPtr = &dataVec[0];
		int bitPos = 0;

		for (uint i = 0; i < numElements; i++)
		{
			if (32 - bitPos >= numBits)
			{
				uint val;
				memcpy(&val, srcPtr, sizeof(uint));
				uint n = val << bitPos;
				*dstPtr++ = n >> (32 - numBits);

				bitPos += numBits;
				if (bitPos == 32) // shift >= 32 is undefined
				{
					bitPos = 0;
					srcPtr++;
				}
			}
			else
			{
				uint val;
				memcpy(&val, srcPtr, sizeof(uint));
				srcPtr++;
				uint n = val << bitPos;
				*dstPtr = n >> (32 - numBits);
				bitPos -= (32 - numBits);
				memcpy(&val, srcPtr, sizeof(uint));
				*dstPtr++ |= val >> (32 - bitPos);
			}
		}

		if (numBytesNotNeeded > 0)
			memcpy(srcPtr, &lastUInt, sizeof(uint)); // restore the last UInt

		*ppByte += numBytes - numBytesNotNeeded;
	}
	return true;
}


bool cBitStuffer::ReadUInt(BYTE** ppByte, uint& k, int numBytes)
{
	BYTE * ptr = *ppByte;

	if (numBytes == 1)
	{
		k = *ptr;
	}
	else if (numBytes == 2)
	{
		unsigned short s;
		memcpy(&s, ptr, sizeof(unsigned short));
		k = s;
	}
	else if (numBytes == 4)
	{
		memcpy(&k, ptr, sizeof(unsigned int));
	}
	else
		return false;

	*ppByte = ptr + numBytes;
	return true;
}


uint cBitStuffer::NumTailBytesNotNeeded(uint numElem, int numBits)
{
	const int numBitsTail = (numElem * numBits) & 31;
	const int numBytesTail = (numBitsTail + 7) >> 3;
	return (numBytesTail > 0) ? 4 - numBytesTail : 0;
}
