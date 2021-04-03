//
// 2021-04-03, jjuiddong
// esri::lerc cntzimage format uncompress
// cntzimage:
//	- RLE compress format
// reference
//	- esri::lerc
//
#pragma once


namespace gis2
{

	class cCntZImg
	{
	public:
		enum class DataType { DT_Char, DT_Byte, DT_Short, DT_UShort, DT_Int, DT_UInt, DT_Float, DT_Double, DT_Undefined };
		enum class Type { BYTE, RGB, SHORT, LONG, FLOAT, DOUBLE, COMPLEX, POINT3F, CNT_Z, CNT_ZXY, Last_Type_ };
		class sCntZ
		{
		public:
			float cnt, z;
			bool operator == (const sCntZ& cz) const { return cnt == cz.cnt && z == cz.z; }
			bool operator != (const sCntZ& cz) const { return cnt != cz.cnt || z != cz.z; }
			void operator += (const sCntZ& cz) { cnt += cz.cnt;  z += cz.z; }
		};

		cCntZImg();
		virtual ~cCntZImg();

		float* UnCompress(const string &fileName);
		bool UnCompress(const BYTE* pLercBlob, uint blobSize
			, BYTE* pValidBytes, uint nDim, uint nCols, uint nRows
			, int nBands, uint dataType, void* pData);
		void Clear();


	protected:
		bool DecodeTempl(float *pData, const BYTE* pLercBlob, uint numBytesBlob
			, uint nDim, uint nCols, uint nRows, int nBands);
		bool ReadCntZImg(BYTE** ppByte, double maxZError, 
			bool onlyHeader, bool onlyZPart);
		bool Convert(float* arr);
		bool ReadTiles(bool zPart, double maxZErrorInFile, uint numTilesVert
			, uint numTilesHori, float maxValInImg, BYTE* bArr);
		bool ReadZTile(BYTE** ppByte, uint i0, uint i1, uint j0, uint j1
			, double maxZErrorInFile, float maxZInImg);
		bool ReadFlt(BYTE** ppByte, float& z, uint numBytes);
		bool ResizeFill0(uint width, uint height);
		bool Resize(uint width, uint height);
		uint ComputeNumBytesNeededToReadHeader(bool onlyZPart);
		string GetTypeString() { return "CntZImage "; }
		sCntZ* GetData() { return m_data; }
		uint GetSize() const { return m_width * m_height; }


	public:
		Type m_type;
		uint m_width;
		uint m_height;
		sCntZ *m_data;
		vector<uint> m_tmpDataVec; // used in read fcts
		bool m_bDecoderCanIgnoreMask;
	};


	//--------------------------------------------------------------------------------
	// esri::Lerc BitStuffer
	class cBitStuffer
	{
	public:
		cBitStuffer() {}
		virtual ~cBitStuffer() {}
		static bool Read(BYTE** ppByte, vector<uint>& dataVec);
	protected:
		static bool ReadUInt(BYTE** ppByte, uint& k, int numBytes); // numBytes = 1, 2, or 4
		static uint NumTailBytesNotNeeded(uint numElem, int numBits);
	};


}
