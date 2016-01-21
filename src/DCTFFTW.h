#ifndef __MV_DCTFFTW__
#define __MV_DCTFFTW__
#include "fftw3.h"
#include "DCT.h"

class DCTFFTW : public DCTClass {
	float * fSrc;
	fftwf_plan dctplan;
	float * fSrcDCT;
	int dctshift;
	int dctshift0;
	template <typename PixelType>
	void Bytes2Float(const uint8_t * srcp0, int _pitch, float * realdata);
	template <typename PixelType>
	void Float2Bytes(uint8_t * srcp0, int _pitch, float * realdata);
public:
	DCTFFTW(int _sizex, int _sizey, int _dctmode);
	~DCTFFTW();
	virtual void DCTBytes2D(const uint8_t *srcp0, int _src_pitch,
		uint8_t *dctp, int _dct_pitch) override;
};

#endif