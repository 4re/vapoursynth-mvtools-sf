#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "VapourSynth.h"
#include "VSHelper.h"
#include "MaskFun.hpp"
#include "MVFilter.hpp"
#include "SimpleResize.hpp"

struct MVFlowBlurData {
	VSNodeRef *node;
	const VSVideoInfo *vi;
	VSNodeRef *finest;
	VSNodeRef *super;
	VSNodeRef *mvbw;
	VSNodeRef *mvfw;
	double blur;
	int32_t prec;
	double thscd1;
	double thscd2;
	MVClipDicks *mvClipB;
	MVClipDicks *mvClipF;
	MVFilter *bleh;
	int32_t nSuperHPad;
	int32_t nWidthUV;
	int32_t nHeightUV;
	int32_t nVPaddingUV;
	int32_t nHPaddingUV;
	int32_t VPitchY;
	int32_t VPitchUV;
	int32_t blur256;
	SimpleResize<int32_t> *upsizer;
	SimpleResize<int32_t> *upsizerUV;
};

static void VS_CC mvflowblurInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi) {
	MVFlowBlurData *d = reinterpret_cast<MVFlowBlurData *>(*instanceData);
	vsapi->setVideoInfo(d->vi, 1, node);
}

template <typename PixelType>
static void RealFlowBlur(uint8_t *pdst8, int32_t dst_pitch, const uint8_t *pref8, int32_t ref_pitch,
	int32_t *VXFullB, int32_t *VXFullF, int32_t *VYFullB, int32_t *VYFullF,
	int32_t VPitch, int32_t width, int32_t height, int32_t blur256, int32_t prec, int32_t nPel) {
	const PixelType *pref = (const PixelType *)pref8;
	PixelType *pdst = (PixelType *)pdst8;
	ref_pitch /= sizeof(PixelType);
	dst_pitch /= sizeof(PixelType);
	if (nPel == 1) {
		for (int32_t h = 0; h<height; h++) {
			for (int32_t w = 0; w<width; w++) {
				double bluredsum = pref[w];
				int32_t vxF0 = VXFullF[w] * blur256;
				int32_t vyF0 = VYFullF[w] * blur256;
				int32_t mF = (std::max(abs(vxF0), abs(vyF0)) / prec) >> 8;
				if (mF>0) {
					vxF0 /= mF;
					vyF0 /= mF;
					int32_t vxF = vxF0;
					int32_t vyF = vyF0;
					for (int32_t i = 0; i<mF; i++) {
						double dstF = pref[(vyF >> 8)*ref_pitch + (vxF >> 8) + w];
						bluredsum += dstF;
						vxF += vxF0;
						vyF += vyF0;
					}
				}
				int32_t vxB0 = VXFullB[w] * blur256;
				int32_t vyB0 = VYFullB[w] * blur256;
				int32_t mB = (std::max(abs(vxB0), abs(vyB0)) / prec) >> 8;
				if (mB>0) {
					vxB0 /= mB;
					vyB0 /= mB;
					int32_t vxB = vxB0;
					int32_t vyB = vyB0;
					for (int32_t i = 0; i<mB; i++)
					{
						double dstB = pref[(vyB >> 8)*ref_pitch + (vxB >> 8) + w];
						bluredsum += dstB;
						vxB += vxB0;
						vyB += vyB0;
					}
				}
				pdst[w] = static_cast<PixelType>(bluredsum / (mF + mB + 1));
			}
			pdst += dst_pitch;
			pref += ref_pitch;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
		}
	}
	else if (nPel == 2)
	{
		for (int32_t h = 0; h<height; h++)
		{
			for (int32_t w = 0; w<width; w++)
			{
				double bluredsum = pref[w << 1];
				int32_t vxF0 = VXFullF[w] * blur256;
				int32_t vyF0 = VYFullF[w] * blur256;
				int32_t mF = (std::max(abs(vxF0), abs(vyF0)) / prec) >> 8;
				if (mF>0)
				{
					vxF0 /= mF;
					vyF0 /= mF;
					int32_t vxF = vxF0;
					int32_t vyF = vyF0;
					for (int32_t i = 0; i<mF; i++)
					{
						double dstF = pref[(vyF >> 8)*ref_pitch + (vxF >> 8) + (w << 1)];
						bluredsum += dstF;
						vxF += vxF0;
						vyF += vyF0;
					}
				}
				int32_t vxB0 = VXFullB[w] * blur256;
				int32_t vyB0 = VYFullB[w] * blur256;
				int32_t mB = (std::max(abs(vxB0), abs(vyB0)) / prec) >> 8;
				if (mB>0)
				{
					vxB0 /= mB;
					vyB0 /= mB;
					int32_t vxB = vxB0;
					int32_t vyB = vyB0;
					for (int32_t i = 0; i<mB; i++)
					{
						double dstB = pref[(vyB >> 8)*ref_pitch + (vxB >> 8) + (w << 1)];
						bluredsum += dstB;
						vxB += vxB0;
						vyB += vyB0;
					}
				}
				pdst[w] = static_cast<PixelType>(bluredsum / (mF + mB + 1));
			}
			pdst += dst_pitch;
			pref += (ref_pitch << 1);
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
		}
	}
	else if (nPel == 4)
	{
		for (int32_t h = 0; h<height; h++)
		{
			for (int32_t w = 0; w<width; w++)
			{
				double bluredsum = pref[w << 2];
				int32_t vxF0 = VXFullF[w] * blur256;
				int32_t vyF0 = VYFullF[w] * blur256;
				int32_t mF = (std::max(abs(vxF0), abs(vyF0)) / prec) >> 8;
				if (mF>0)
				{
					vxF0 /= mF;
					vyF0 /= mF;
					int32_t vxF = vxF0;
					int32_t vyF = vyF0;
					for (int32_t i = 0; i<mF; i++)
					{
						double dstF = pref[(vyF >> 8)*ref_pitch + (vxF >> 8) + (w << 2)];
						bluredsum += dstF;
						vxF += vxF0;
						vyF += vyF0;
					}
				}
				int32_t vxB0 = VXFullB[w] * blur256;
				int32_t vyB0 = VYFullB[w] * blur256;
				int32_t mB = (std::max(abs(vxB0), abs(vyB0)) / prec) >> 8;
				if (mB>0)
				{
					vxB0 /= mB;
					vyB0 /= mB;
					int32_t vxB = vxB0;
					int32_t vyB = vyB0;
					for (int32_t i = 0; i<mB; i++)
					{
						double dstB = pref[(vyB >> 8)*ref_pitch + (vxB >> 8) + (w << 2)];
						bluredsum += dstB;
						vxB += vxB0;
						vyB += vyB0;
					}
				}
				pdst[w] = static_cast<PixelType>(bluredsum / (mF + mB + 1));
			}
			pdst += dst_pitch;
			pref += (ref_pitch << 2);
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
		}
	}
}


static void FlowBlur(uint8_t *pdst, int32_t dst_pitch, const uint8_t *pref, int32_t ref_pitch,
	int32_t*VXFullB, int32_t *VXFullF, int32_t *VYFullB, int32_t *VYFullF,
	int32_t VPitch, int32_t width, int32_t height, int32_t blur256, int32_t prec, int32_t nPel) {
	RealFlowBlur<float>(pdst, dst_pitch, pref, ref_pitch, VXFullB, VXFullF, VYFullB, VYFullF, VPitch, width, height, blur256, prec, nPel);
}


static const VSFrameRef *VS_CC mvflowblurGetFrame(int32_t n, int32_t activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi) {
	MVFlowBlurData *d = reinterpret_cast<MVFlowBlurData *>(*instanceData);
	if (activationReason == arInitial) {
		int32_t off = d->mvClipB->GetDeltaFrame(); // integer offset of reference frame

		if (n - off >= 0 && (n + off < d->vi->numFrames || !d->vi->numFrames)) {
			vsapi->requestFrameFilter(n - off, d->mvbw, frameCtx);
			vsapi->requestFrameFilter(n + off, d->mvfw, frameCtx);
		}

		vsapi->requestFrameFilter(n, d->finest, frameCtx);
		vsapi->requestFrameFilter(n, d->node, frameCtx);
	}
	else if (activationReason == arAllFramesReady) {
		uint8_t *pDst[3];
		const uint8_t *pRef[3];
		int32_t nDstPitches[3];
		int32_t nRefPitches[3];

		MVClipBalls ballsF(d->mvClipF, vsapi);
		MVClipBalls ballsB(d->mvClipB, vsapi);

		bool isUsableB = false;
		bool isUsableF = false;

		int32_t off = d->mvClipB->GetDeltaFrame(); // integer offset of reference frame

		if (n - off >= 0 && (n + off < d->vi->numFrames || !d->vi->numFrames)) {
			const VSFrameRef *mvF = vsapi->getFrameFilter(n + off, d->mvfw, frameCtx);
			ballsF.Update(mvF);
			vsapi->freeFrame(mvF);
			isUsableF = ballsF.IsUsable();

			const VSFrameRef *mvB = vsapi->getFrameFilter(n - off, d->mvbw, frameCtx);
			ballsB.Update(mvB);
			vsapi->freeFrame(mvB);
			isUsableB = ballsB.IsUsable();
		}


		if (isUsableB && isUsableF)
		{
			const VSFrameRef *ref = vsapi->getFrameFilter(n, d->finest, frameCtx); //  ref for  compensation
			VSFrameRef *dst = vsapi->newVideoFrame(d->vi->format, d->vi->width, d->vi->height, ref, core);

			for (int32_t i = 0; i < d->vi->format->numPlanes; i++) {
				pDst[i] = vsapi->getWritePtr(dst, i);
				pRef[i] = vsapi->getReadPtr(ref, i);
				nDstPitches[i] = vsapi->getStride(dst, i);
				nRefPitches[i] = vsapi->getStride(ref, i);
			}

			const int32_t nWidth = d->bleh->nWidth;
			const int32_t nHeight = d->bleh->nHeight;
			const int32_t nWidthUV = d->nWidthUV;
			const int32_t nHeightUV = d->nHeightUV;
			const int32_t xRatioUV = d->bleh->xRatioUV;
			const int32_t yRatioUV = d->bleh->yRatioUV;
			const int32_t nBlkX = d->bleh->nBlkX;
			const int32_t nBlkY = d->bleh->nBlkY;
			const int32_t nVPadding = d->bleh->nVPadding;
			const int32_t nHPadding = d->bleh->nHPadding;
			const int32_t nVPaddingUV = d->nVPaddingUV;
			const int32_t nHPaddingUV = d->nHPaddingUV;
			const int32_t nPel = d->bleh->nPel;
			const int32_t blur256 = d->blur256;
			const int32_t prec = d->prec;
			const int32_t VPitchY = d->VPitchY;
			const int32_t VPitchUV = d->VPitchUV;

			int32_t bytesPerSample = d->vi->format->bytesPerSample;

			int32_t nOffsetY = nRefPitches[0] * nVPadding * nPel + nHPadding * bytesPerSample * nPel;
			int32_t nOffsetUV = nRefPitches[1] * nVPaddingUV * nPel + nHPaddingUV * bytesPerSample * nPel;


			auto *VXFullYB = new int32_t[nHeight * VPitchY];
			auto *VYFullYB = new int32_t[nHeight * VPitchY];
			auto *VXFullYF = new int32_t[nHeight * VPitchY];
			auto *VYFullYF = new int32_t[nHeight * VPitchY];
			auto *VXSmallYB = new int32_t[nBlkX * nBlkY];
			auto *VYSmallYB = new int32_t[nBlkX * nBlkY];
			auto *VXSmallYF = new int32_t[nBlkX * nBlkY];
			auto *VYSmallYF = new int32_t[nBlkX * nBlkY];

			MakeVectorSmallMasks(ballsB, nBlkX, nBlkY, VXSmallYB, nBlkX, VYSmallYB, nBlkX);
			MakeVectorSmallMasks(ballsF, nBlkX, nBlkY, VXSmallYF, nBlkX, VYSmallYF, nBlkX);


			d->upsizer->Resize(VXFullYB, VPitchY, VXSmallYB, nBlkX, true);
			d->upsizer->Resize(VYFullYB, VPitchY, VYSmallYB, nBlkX, false);
			d->upsizer->Resize(VXFullYF, VPitchY, VXSmallYF, nBlkX, true);
			d->upsizer->Resize(VYFullYF, VPitchY, VYSmallYF, nBlkX, false);

			FlowBlur(pDst[0], nDstPitches[0], pRef[0] + nOffsetY, nRefPitches[0],
				VXFullYB, VXFullYF, VYFullYB, VYFullYF, VPitchY,
				nWidth, nHeight, blur256, prec, nPel);

			if (d->vi->format->colorFamily != cmGray) {
				auto *VXFullUVB = new int32_t[nHeightUV * VPitchUV];
				auto *VYFullUVB = new int32_t[nHeightUV * VPitchUV];

				auto *VXFullUVF = new int32_t[nHeightUV * VPitchUV];
				auto *VYFullUVF = new int32_t[nHeightUV * VPitchUV];

				auto *VXSmallUVB = new int32_t[nBlkX * nBlkY];
				auto *VYSmallUVB = new int32_t[nBlkX * nBlkY];

				auto *VXSmallUVF = new int32_t[nBlkX * nBlkY];
				auto *VYSmallUVF = new int32_t[nBlkX * nBlkY];

				VectorSmallMaskYToHalfUV(VXSmallYB, nBlkX, nBlkY, VXSmallUVB, xRatioUV);
				VectorSmallMaskYToHalfUV(VYSmallYB, nBlkX, nBlkY, VYSmallUVB, yRatioUV);

				VectorSmallMaskYToHalfUV(VXSmallYF, nBlkX, nBlkY, VXSmallUVF, xRatioUV);
				VectorSmallMaskYToHalfUV(VYSmallYF, nBlkX, nBlkY, VYSmallUVF, yRatioUV);

				d->upsizerUV->Resize(VXFullUVB, VPitchUV, VXSmallUVB, nBlkX, true);
				d->upsizerUV->Resize(VYFullUVB, VPitchUV, VYSmallUVB, nBlkX, false);

				d->upsizerUV->Resize(VXFullUVF, VPitchUV, VXSmallUVF, nBlkX, true);
				d->upsizerUV->Resize(VYFullUVF, VPitchUV, VYSmallUVF, nBlkX, false);


				FlowBlur(pDst[1], nDstPitches[1], pRef[1] + nOffsetUV, nRefPitches[1],
					VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, VPitchUV,
					nWidthUV, nHeightUV, blur256, prec, nPel);
				FlowBlur(pDst[2], nDstPitches[2], pRef[2] + nOffsetUV, nRefPitches[2],
					VXFullUVB, VXFullUVF, VYFullUVB, VYFullUVF, VPitchUV,
					nWidthUV, nHeightUV, blur256, prec, nPel);

				delete[] VXFullUVB;
				delete[] VYFullUVB;
				delete[] VXSmallUVB;
				delete[] VYSmallUVB;
				delete[] VXFullUVF;
				delete[] VYFullUVF;
				delete[] VXSmallUVF;
				delete[] VYSmallUVF;
			}

			delete[] VXFullYB;
			delete[] VYFullYB;
			delete[] VXSmallYB;
			delete[] VYSmallYB;
			delete[] VXFullYF;
			delete[] VYFullYF;
			delete[] VXSmallYF;
			delete[] VYSmallYF;

			vsapi->freeFrame(ref);

			return dst;
		}
		else // not usable
		{
			return vsapi->getFrameFilter(n, d->node, frameCtx);
		}
	}

	return nullptr;
}


static void VS_CC mvflowblurFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
	MVFlowBlurData *d = reinterpret_cast<MVFlowBlurData *>(instanceData);

	delete d->mvClipB;
	delete d->mvClipF;

	delete d->bleh;

	delete d->upsizer;
	if (d->vi->format->colorFamily != cmGray)
		delete d->upsizerUV;

	vsapi->freeNode(d->finest);
	vsapi->freeNode(d->super);
	vsapi->freeNode(d->mvfw);
	vsapi->freeNode(d->mvbw);
	vsapi->freeNode(d->node);
	delete d;
}


static void VS_CC mvflowblurCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
	MVFlowBlurData d;
	MVFlowBlurData *data;

	int err;

	d.blur = vsapi->propGetFloat(in, "blur", 0, &err);
	if (err)
		d.blur = 50.;

	d.prec = int64ToIntS(vsapi->propGetInt(in, "prec", 0, &err));
	if (err)
		d.prec = 1;

	d.thscd1 = vsapi->propGetFloat(in, "thscd1", 0, &err);
	if (err)
		d.thscd1 = MV_DEFAULT_SCD1;

	d.thscd2 = vsapi->propGetFloat(in, "thscd2", 0, &err);
	if (err)
		d.thscd2 = MV_DEFAULT_SCD2;


	if (d.blur < 0. || d.blur > 200.) {
		vsapi->setError(out, "FlowBlur: blur must be between 0 and 200 % (inclusive).");
		return;
	}


	if (d.prec < 1) {
		vsapi->setError(out, "FlowBlur: prec must be at least 1.");
		return;
	}
	d.blur256 = static_cast<int32_t>(d.blur * 256. / 200.);
	d.super = vsapi->propGetNode(in, "super", 0, nullptr);
	char errorMsg[1024];
	const VSFrameRef *evil = vsapi->getFrame(0, d.super, errorMsg, 1024);
	if (!evil) {
		vsapi->setError(out, std::string("FlowBlur: failed to retrieve first frame from super clip. Error message: ").append(errorMsg).c_str());
		vsapi->freeNode(d.super);
		return;
	}
	const VSMap *props = vsapi->getFramePropsRO(evil);
	int32_t evil_err[2];
	int32_t nHeightS = int64ToIntS(vsapi->propGetInt(props, "Super_height", 0, &evil_err[0]));
	d.nSuperHPad = int64ToIntS(vsapi->propGetInt(props, "Super_hpad", 0, &evil_err[1]));
	vsapi->freeFrame(evil);

	for (int32_t i = 0; i < 2; i++)
		if (evil_err[i]) {
			vsapi->setError(out, "FlowBlur: required properties not found in first frame of super clip. Maybe clip didn't come from mv.Super? Was the first frame trimmed away?");
			vsapi->freeNode(d.super);
			return;
		}


	d.mvbw = vsapi->propGetNode(in, "mvbw", 0, nullptr);
	d.mvfw = vsapi->propGetNode(in, "mvfw", 0, nullptr);

	// XXX Fuck all this trying.
	try {
		d.mvClipB = new MVClipDicks(d.mvbw, d.thscd1, d.thscd2, vsapi);
	}
	catch (MVException &e) {
		vsapi->setError(out, std::string("FlowBlur: ").append(e.what()).c_str());
		vsapi->freeNode(d.super);
		vsapi->freeNode(d.mvbw);
		vsapi->freeNode(d.mvfw);
		return;
	}

	try {
		d.mvClipF = new MVClipDicks(d.mvfw, d.thscd1, d.thscd2, vsapi);
	}
	catch (MVException &e) {
		vsapi->setError(out, std::string("FlowBlur: ").append(e.what()).c_str());
		vsapi->freeNode(d.super);
		vsapi->freeNode(d.mvfw);
		vsapi->freeNode(d.mvbw);
		delete d.mvClipB;
		return;
	}

	// XXX Alternatively, use both clips' delta as offsets in GetFrame.
	if (d.mvClipF->GetDeltaFrame() != d.mvClipB->GetDeltaFrame()) {
		vsapi->setError(out, "FlowBlur: mvbw and mvfw must be generated with the same delta.");
		vsapi->freeNode(d.super);
		vsapi->freeNode(d.mvfw);
		vsapi->freeNode(d.mvbw);
		delete d.mvClipB;
		delete d.mvClipF;
		return;
	}

	// Make sure the motion vector clips are correct.
	if (!d.mvClipB->IsBackward() || d.mvClipF->IsBackward()) {
		if (!d.mvClipB->IsBackward())
			vsapi->setError(out, "FlowBlur: mvbw must be generated with isb=True.");
		else
			vsapi->setError(out, "FlowBlur: mvfw must be generated with isb=False.");
		vsapi->freeNode(d.super);
		vsapi->freeNode(d.mvfw);
		vsapi->freeNode(d.mvbw);
		delete d.mvClipB;
		delete d.mvClipF;
		return;
	}

	try {
		d.bleh = new MVFilter(d.mvfw, "FlowBlur", vsapi);
	}
	catch (MVException &e) {
		vsapi->setError(out, std::string("FlowBlur: ").append(e.what()).c_str());
		vsapi->freeNode(d.super);
		vsapi->freeNode(d.mvfw);
		vsapi->freeNode(d.mvbw);
		delete d.mvClipB;
		delete d.mvClipF;
		return;
	}

	try {
		// So it checks the similarity of mvfw and mvfw? ?????
		// Copied straight from 2.5.11.3...
		d.bleh->CheckSimilarity(d.mvClipF, "mvfw");
		d.bleh->CheckSimilarity(d.mvClipB, "mvbw");
	}
	catch (MVException &e) {
		vsapi->setError(out, std::string("FlowBlur: ").append(e.what()).c_str());
		delete d.bleh;
		delete d.mvClipB;
		delete d.mvClipF;
		vsapi->freeNode(d.super);
		vsapi->freeNode(d.mvfw);
		vsapi->freeNode(d.mvbw);
		return;
	}

	if (d.bleh->nPel == 1)
		d.finest = vsapi->cloneNodeRef(d.super); // v2.0.9.1
	else
	{
		VSPlugin *mvtoolsPlugin = vsapi->getPluginById("com.zonked.mvsf", core);
		VSPlugin *stdPlugin = vsapi->getPluginById("com.vapoursynth.std", core);

		VSMap *args = vsapi->createMap();
		vsapi->propSetNode(args, "super", d.super, paReplace);
		VSMap *ret = vsapi->invoke(mvtoolsPlugin, "Finest", args);
		if (vsapi->getError(ret)) {
			vsapi->setError(out, std::string("FlowBlur: ").append(vsapi->getError(ret)).c_str());

			delete d.bleh;
			delete d.mvClipB;
			delete d.mvClipF;
			vsapi->freeNode(d.super);
			vsapi->freeNode(d.mvfw);
			vsapi->freeNode(d.mvbw);
			vsapi->freeMap(args);
			vsapi->freeMap(ret);
			return;
		}
		d.finest = vsapi->propGetNode(ret, "clip", 0, nullptr);
		vsapi->freeMap(ret);

		vsapi->clearMap(args);
		vsapi->propSetNode(args, "clip", d.finest, paReplace);
		vsapi->freeNode(d.finest);
		ret = vsapi->invoke(stdPlugin, "Cache", args);
		vsapi->freeMap(args);
		if (vsapi->getError(ret)) {
			// prefix the error messages
			vsapi->setError(out, std::string("FlowBlur: ").append(vsapi->getError(ret)).c_str());

			delete d.bleh;
			delete d.mvClipB;
			delete d.mvClipF;
			vsapi->freeNode(d.super);
			vsapi->freeNode(d.mvfw);
			vsapi->freeNode(d.mvbw);
			vsapi->freeMap(ret);
			return;
		}
		d.finest = vsapi->propGetNode(ret, "clip", 0, nullptr);
		vsapi->freeMap(ret);
	}

	d.node = vsapi->propGetNode(in, "clip", 0, 0);
	d.vi = vsapi->getVideoInfo(d.node);

	const VSVideoInfo *supervi = vsapi->getVideoInfo(d.super);
	int32_t nSuperWidth = supervi->width;

	if (d.bleh->nHeight != nHeightS || d.bleh->nWidth != nSuperWidth - d.nSuperHPad * 2) {
		vsapi->setError(out, "FlowBlur: wrong source or super clip frame size.");
		vsapi->freeNode(d.finest);
		vsapi->freeNode(d.super);
		vsapi->freeNode(d.mvfw);
		vsapi->freeNode(d.mvbw);
		vsapi->freeNode(d.node);
		delete d.bleh;
		delete d.mvClipB;
		delete d.mvClipF;
		return;
	}

	if (!isConstantFormat(d.vi) || d.vi->format->bitsPerSample < 32 || d.vi->format->sampleType != stFloat) {
		vsapi->setError(out, "FlowBlur: input clip must be single precision fp, with constant dimensions.");
		vsapi->freeNode(d.super);
		vsapi->freeNode(d.finest);
		vsapi->freeNode(d.mvfw);
		vsapi->freeNode(d.mvbw);
		vsapi->freeNode(d.node);
		delete d.bleh;
		delete d.mvClipB;
		delete d.mvClipF;
		return;
	}

	d.nHeightUV = d.bleh->nHeight / d.bleh->yRatioUV;
	d.nWidthUV = d.bleh->nWidth / d.bleh->xRatioUV;
	d.nHPaddingUV = d.bleh->nHPadding / d.bleh->xRatioUV;
	d.nVPaddingUV = d.bleh->nVPadding / d.bleh->yRatioUV;
	d.VPitchY = d.bleh->nWidth;
	d.VPitchUV = d.nWidthUV;
	d.upsizer = new SimpleResize<int32_t>(d.bleh->nWidth, d.bleh->nHeight, d.bleh->nBlkX, d.bleh->nBlkY, d.mvClipB->nWidth, d.mvClipB->nHeight, d.mvClipB->nPel);
	if (d.vi->format->colorFamily != cmGray)
		d.upsizerUV = new SimpleResize<int32_t>(d.nWidthUV, d.nHeightUV, d.bleh->nBlkX, d.bleh->nBlkY, d.nWidthUV, d.nHeightUV, d.mvClipB->nPel);
	data = new MVFlowBlurData;
	*data = d;
	vsapi->createFilter(in, out, "FlowBlur", mvflowblurInit, mvflowblurGetFrame, mvflowblurFree, fmParallel, 0, data, core);
}

void mvflowblurRegister(VSRegisterFunction registerFunc, VSPlugin *plugin) {
	registerFunc("FlowBlur",
		"clip:clip;"
		"super:clip;"
		"mvbw:clip;"
		"mvfw:clip;"
		"blur:float:opt;"
		"prec:int:opt;"
		"thscd1:float:opt;"
		"thscd2:float:opt;"
		, mvflowblurCreate, 0, plugin);
}
