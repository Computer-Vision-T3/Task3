#include "SIFTDescriptor.h"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr int   ORI_BINS      = 36;
static constexpr float ORI_RADIUS    = 4.5f;   // 3.0 * 1.5
static constexpr float ORI_PEAK      = 0.8f;
static constexpr int   DESC_CELLS    = 4;
static constexpr int   DESC_BINS     = 8;
static constexpr float DESC_MAG_THR  = 0.2f;
static constexpr float DESC_SCALE    = 512.f;
static constexpr int   MAX_STEPS     = 5;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static inline float at(const cv::Mat& m, int r, int c)   { return m.at<float>(r, c); }
static inline bool  inBounds(const cv::Mat& m, int r, int c)
{ return r > 0 && r < m.rows-1 && c > 0 && c < m.cols-1; }

// ---------------------------------------------------------------------------
// drawKeypoints
// ---------------------------------------------------------------------------
cv::Mat SIFTDescriptor::drawKeypoints(const cv::Mat& img,
                                      const std::vector<cv::KeyPoint>& kps)
{
    cv::Mat out;
    if (img.channels() == 1) {
        cv::cvtColor(img, out, cv::COLOR_GRAY2BGR);
    } else {
        out = img.clone();
    }

    for (const auto& kp : kps) {
        int r = std::max(1, static_cast<int>(kp.size / 2));
        float a = kp.angle * (float)CV_PI / 180.f;
        cv::Point2f c(kp.pt), tip(c.x + r*std::cos(a), c.y + r*std::sin(a));
        cv::circle(out, c,   r, {0,255,0}, 1, cv::LINE_AA);
        cv::line  (out, c, tip, {0,0,255}, 1, cv::LINE_AA);
        cv::circle(out, c,   2, {255,0,0}, -1);
    }
    return out;
}

// ---------------------------------------------------------------------------
// buildGaussianPyramid
// ---------------------------------------------------------------------------
SIFTDescriptor::Pyramid
SIFTDescriptor::buildGaussianPyramid(const cv::Mat& base, int nOct, int nLay, double sig0)
{
    double k = std::pow(2.0, 1.0 / nLay);
    Pyramid pyr(nOct);

    for (int o = 0; o < nOct; ++o) {
        pyr[o].resize(nLay + 3);

        if (o == 0) {
            pyr[o][0] = base.clone();
        } else {
            cv::resize(pyr[o - 1][nLay], pyr[o][0], cv::Size(), 0.5, 0.5, cv::INTER_NEAREST);
        }

        for (int s = 1; s < nLay + 3; ++s) {
            double sp = sig0 * std::pow(k, s - 1);
            double sig = std::sqrt((sp * k) * (sp * k) - sp * sp);
            cv::GaussianBlur(pyr[o][s - 1], pyr[o][s], {0, 0}, sig);
        }
    }

    return pyr;
}

// ---------------------------------------------------------------------------
// buildDoGPyramid
// ---------------------------------------------------------------------------
SIFTDescriptor::Pyramid
SIFTDescriptor::buildDoGPyramid(const Pyramid& g)
{
    Pyramid dog(g.size());
    for (int o = 0; o < (int)g.size(); ++o) {
        dog[o].resize(g[o].size()-1);
        for (int s = 0; s < (int)dog[o].size(); ++s)
            cv::subtract(g[o][s+1], g[o][s], dog[o][s]);
    }
    return dog;
}

// ---------------------------------------------------------------------------
// localiseExtrema
// ---------------------------------------------------------------------------

bool SIFTDescriptor::localiseExtrema(const Pyramid& dog, int oct, int lay, int r, int c,
                                     int nLay, double cThr, double eThr, double sig0,
                                     cv::KeyPoint& kp)
{
    int nD = (int)dog[oct].size();
    float xs=0, xr=0, xc=0;

    for (int step=0; step<MAX_STEPS; ++step) {
        if (lay<1 || lay>nD-2) return false;
        const cv::Mat& P=dog[oct][lay-1], &C=dog[oct][lay], &N=dog[oct][lay+1];
        if (!inBounds(C,r,c)) return false;

        float v = at(C,r,c);
        float dD[3] = { 0.5f*(at(N,r,c)-at(P,r,c)),
                        0.5f*(at(C,r+1,c)-at(C,r-1,c)),
                        0.5f*(at(C,r,c+1)-at(C,r,c-1)) };

        float dss = at(N,r,c)+at(P,r,c)-2*v,   drr = at(C,r+1,c)+at(C,r-1,c)-2*v,
              dcc = at(C,r,c+1)+at(C,r,c-1)-2*v,
              drs = 0.25f*(at(N,r+1,c)-at(N,r-1,c)-at(P,r+1,c)+at(P,r-1,c)),
              drc = 0.25f*(at(C,r+1,c+1)-at(C,r+1,c-1)-at(C,r-1,c+1)+at(C,r-1,c-1)),
              dsc = 0.25f*(at(N,r,c+1)-at(N,r,c-1)-at(P,r,c+1)+at(P,r,c-1));

        cv::Vec3f x;
        if (!cv::solve(cv::Mat(cv::Matx33f(dss,drs,dsc,drs,drr,drc,dsc,drc,dcc)),
                       cv::Mat(cv::Vec3f(-dD[0],-dD[1],-dD[2])), cv::Mat(x), cv::DECOMP_LU))
            return false;
        xs=x[0]; xr=x[1]; xc=x[2];
        if (std::abs(xs)<0.5f && std::abs(xr)<0.5f && std::abs(xc)<0.5f) break;
        c+=std::round(xc); r+=std::round(xr); lay+=std::round(xs);
    }

    // Contrast check — re-validate position after interpolation loop
    if (lay<1 || lay>nD-2 || !inBounds(dog[oct][lay],r,c)) return false;
    float dDs = 0.5f*(at(dog[oct][lay+1],r,c)-at(dog[oct][lay-1],r,c));
    float dDr = 0.5f*(at(dog[oct][lay],r+1,c)-at(dog[oct][lay],r-1,c));
    float dDc = 0.5f*(at(dog[oct][lay],r,c+1)-at(dog[oct][lay],r,c-1));
    float contr = at(dog[oct][lay],r,c) + 0.5f*(dDs*xs+dDr*xr+dDc*xc);
    // Keep contrast gating stable when UI changes nLay.
    // Using a fixed reference layer count avoids inflated keypoint counts
    // just because scale sampling density increased.
    constexpr float kRefLayers = 3.0f;
    if (std::abs(contr) < (float)cThr / kRefLayers) return false;

    // Edge check
    const cv::Mat& D = dog[oct][lay];
    float ev=at(D,r,c), dxx=at(D,r,c+1)+at(D,r,c-1)-2*ev,
          dyy=at(D,r+1,c)+at(D,r-1,c)-2*ev,
          dxy=0.25f*(at(D,r+1,c+1)-at(D,r+1,c-1)-at(D,r-1,c+1)+at(D,r-1,c-1));
    float det=dxx*dyy-dxy*dxy;
    if (det<=0 || (dxx+dyy)*(dxx+dyy)/det >= (eThr+1)*(eThr+1)/eThr) return false;

    float sc = std::pow(2.f, oct);
    kp.pt = { (float)((c+xc)*sc), (float)((r+xr)*sc) };
    kp.octave = oct + (lay<<8) + ((int)std::round((xs+0.5f)*255)<<16);
    kp.size = (float)(2.0*sig0*std::pow(2.0,(lay+xs)/nLay)*sc);
    kp.response = std::abs(contr);
    kp.angle = -1.f;
    return true;
}

// ---------------------------------------------------------------------------
// detectExtrema
// ---------------------------------------------------------------------------
std::vector<cv::KeyPoint>
SIFTDescriptor::detectExtrema(const Pyramid& dog, int nLay, double cThr, double eThr,
                               double sig0, int imgRows, int imgCols)
{
    std::vector<cv::KeyPoint> kps;
    for (int o=0; o<(int)dog.size(); ++o)
    for (int s=1; s<=nLay && s<(int)dog[o].size()-1; ++s) {
        const auto& P=dog[o][s-1], &C=dog[o][s], &N=dog[o][s+1];
        for (int r=1; r<C.rows-1; ++r)
        for (int c=1; c<C.cols-1; ++c) {
            float v = at(C,r,c);
            bool isMax = v>0, isMin = v<0;
            for (int dr=-1; dr<=1 && (isMax||isMin); ++dr)
            for (int dc=-1; dc<=1 && (isMax||isMin); ++dc) {
                if (isMax && (at(P,r+dr,c+dc)>=v || at(N,r+dr,c+dc)>=v ||
                    ((dr||dc) && at(C,r+dr,c+dc)>=v))) isMax=false;
                if (isMin && (at(P,r+dr,c+dc)<=v || at(N,r+dr,c+dc)<=v ||
                    ((dr||dc) && at(C,r+dr,c+dc)<=v))) isMin=false;
            }
            if (!isMax && !isMin) continue;
            cv::KeyPoint kp;
            if (localiseExtrema(dog,o,s,r,c,nLay,cThr,eThr,sig0,kp) &&
                kp.pt.x>=0 && kp.pt.x<imgCols && kp.pt.y>=0 && kp.pt.y<imgRows)
                kps.push_back(kp);
        }
    }
    return kps;
}

// ---------------------------------------------------------------------------
// assignOrientations
// ---------------------------------------------------------------------------
std::vector<cv::KeyPoint>
SIFTDescriptor::assignOrientations(const std::vector<cv::KeyPoint>& kps,
                                   const Pyramid& gauss, int nLay)
{
    std::vector<cv::KeyPoint> out;
    out.reserve(kps.size()*2);
    for (const auto& kp : kps) {
        int o=kp.octave&0xFF, s=(kp.octave>>8)&0xFF;
        if (o>=(int)gauss.size() || s>=(int)gauss[o].size()) continue;
        const cv::Mat& img = gauss[o][s];
        float sc  = std::pow(2.f,o);
        float ksc = kp.size/(2.f*sc);
        float ri  = std::round(ORI_RADIUS*ksc);
        float sig2= 2.f*1.5f*1.5f*ksc*ksc;
        int cx=(int)std::round(kp.pt.x/sc), cy=(int)std::round(kp.pt.y/sc);

        float hist[ORI_BINS]={};
        for (int dy=-(int)ri; dy<=(int)ri; ++dy)
        for (int dx=-(int)ri; dx<=(int)ri; ++dx) {
            int yr=cy+dy, xc=cx+dx;
            if (!inBounds(img,yr,xc)) continue;
            float gx=at(img,yr,xc+1)-at(img,yr,xc-1);
            float gy=at(img,yr+1,xc)-at(img,yr-1,xc);
            float mag=std::sqrt(gx*gx+gy*gy);
            float ang=std::atan2(gy,gx)*180.f/(float)CV_PI;
            if (ang<0) ang+=360.f;
            float w=std::exp(-(dx*dx+dy*dy)/sig2);
            hist[(int)std::round(ang*ORI_BINS/360.f)%ORI_BINS] += w*mag;
        }
        // Smooth 6 passes
        for (int p=0; p<6; ++p) {
            float tmp[ORI_BINS];
            for (int b=0; b<ORI_BINS; ++b)
                tmp[b]=(hist[(b-1+ORI_BINS)%ORI_BINS]+hist[b]+hist[(b+1)%ORI_BINS])/3.f;
            std::copy(tmp,tmp+ORI_BINS,hist);
        }
        float mx=*std::max_element(hist,hist+ORI_BINS);
        for (int b=0; b<ORI_BINS; ++b) {
            int pv=(b-1+ORI_BINS)%ORI_BINS, nx=(b+1)%ORI_BINS;
            if (hist[b]>=mx*ORI_PEAK && hist[b]>hist[pv] && hist[b]>hist[nx]) {
                float denom = hist[pv]-2.f*hist[b]+hist[nx];
                if (std::abs(denom) < 1e-6f) continue;
                float bi=b+0.5f*(hist[pv]-hist[nx])/denom;
                float ang=bi*360.f/ORI_BINS;
                if (ang<0) ang+=360.f; if (ang>=360.f) ang-=360.f;
                cv::KeyPoint nk=kp; nk.angle=ang;
                out.push_back(nk);
            }
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// computeDescriptors
// ---------------------------------------------------------------------------
cv::Mat SIFTDescriptor::computeDescriptors(const std::vector<cv::KeyPoint>& kps,
                                            const Pyramid& gauss, int nLay)
{
    if (kps.empty()) return {};
    int dLen = DESC_CELLS*DESC_CELLS*DESC_BINS;  // 128
    cv::Mat descs((int)kps.size(), dLen, CV_32F, cv::Scalar(0));

    for (int ki=0; ki<(int)kps.size(); ++ki) {
        const auto& kp=kps[ki];
        int o=kp.octave&0xFF, s=(kp.octave>>8)&0xFF;
        if (o>=(int)gauss.size() || s>=(int)gauss[o].size()) continue;
        const cv::Mat& img=gauss[o][s];
        float sc=std::pow(2.f,o), ksc=kp.size/(2.f*sc);
        float hw=3.f*ksc;
        int rad=(int)(hw*(DESC_CELLS+1)*std::sqrt(2.f)*0.5f+0.5f);
        int cx=(int)std::round(kp.pt.x/sc), cy=(int)std::round(kp.pt.y/sc);
        float ang=-kp.angle*(float)CV_PI/180.f;
        float cA=std::cos(ang), sA=std::sin(ang);
        float* row=descs.ptr<float>(ki);

        for (int dy=-rad; dy<=rad; ++dy)
        for (int dx=-rad; dx<=rad; ++dx) {
            float ry=(dx*sA+dy*cA)/hw, rx=(dx*cA-dy*sA)/hw;
            float bR=ry+DESC_CELLS/2.f-0.5f, bC=rx+DESC_CELLS/2.f-0.5f;
            if (bR<=-1||bR>=DESC_CELLS||bC<=-1||bC>=DESC_CELLS) continue;
            int yr=cy+dy, xc=cx+dx;
            if (!inBounds(img,yr,xc)) continue;
            float gx=at(img,yr,xc+1)-at(img,yr,xc-1), gy=at(img,yr+1,xc)-at(img,yr-1,xc);
            float rgx=gx*cA+gy*sA, rgy=-gx*sA+gy*cA;
            float mag=std::sqrt(rgx*rgx+rgy*rgy);
            float ori=std::atan2(rgy,rgx)*180.f/(float)CV_PI; if(ori<0)ori+=360.f;
            float wx=rx/(DESC_CELLS/2.f), wy=ry/(DESC_CELLS/2.f);
            float w=std::exp(-(wx*wx+wy*wy)/2.f);
            float oB=ori*DESC_BINS/360.f;
            int r0=(int)std::floor(bR), c0=(int)std::floor(bC), o0=(int)std::floor(oB)%DESC_BINS;
            float dr=bR-std::floor(bR), dc=bC-std::floor(bC), dob=oB-std::floor(oB);
            for (int rb=0; rb<=1; ++rb) for (int cb=0; cb<=1; ++cb) for (int ob=0; ob<=1; ++ob) {
                int rr=r0+rb, cc=c0+cb;
                if (rr<0||rr>=DESC_CELLS||cc<0||cc>=DESC_CELLS) continue;
                row[rr*DESC_CELLS*DESC_BINS+cc*DESC_BINS+(o0+ob)%DESC_BINS] +=
                    w*mag * ((rb?dr:1-dr)) * ((cb?dc:1-dc)) * ((ob?dob:1-dob));
            }
        }
        // Normalise → clamp → re-normalise
        float n=1e-7f; for(int i=0;i<dLen;++i) n+=row[i]*row[i]; n=std::sqrt(n);
        for(int i=0;i<dLen;++i) row[i]=std::min(row[i]/n, DESC_MAG_THR);
        n=1e-7f; for(int i=0;i<dLen;++i) n+=row[i]*row[i]; n=std::sqrt(n);
        for(int i=0;i<dLen;++i) row[i]=std::min(std::round(row[i]/n*DESC_SCALE), 255.f);
    }
    return descs;
}

// ---------------------------------------------------------------------------
// filterByResponse
// ---------------------------------------------------------------------------
std::vector<cv::KeyPoint>
SIFTDescriptor::filterByResponse(const std::vector<cv::KeyPoint>& kps, int nFeat)
{
    if (nFeat <= 0) return kps;

    if (nFeat >= static_cast<int>(kps.size())) {
        return {};
    }

    int keepCount = static_cast<int>(kps.size()) - nFeat;

    std::vector<cv::KeyPoint> s=kps;
    std::partial_sort(s.begin(), s.begin()+keepCount, s.end(),
        [](const cv::KeyPoint& a, const cv::KeyPoint& b){ return a.response>b.response; });
    s.resize(keepCount);
    return s;
}

// ---------------------------------------------------------------------------
// describe  (public entry point)
// ---------------------------------------------------------------------------
std::tuple<cv::Mat, std::vector<cv::KeyPoint>, cv::Mat>
SIFTDescriptor::describe(const cv::Mat& src, int nFeat, int nOctave,
                          double contrast, double edge, double sigma,
                          bool colorMode, double& timingMs)
{
    if (src.empty()) throw std::invalid_argument("Input image is empty.");
    auto t0 = std::chrono::high_resolution_clock::now();

    int nOct = std::max(1, static_cast<int>(std::log2(std::min(src.rows, src.cols))) - 2);
    int nLay = std::max(1, nOctave > 0 ? nOctave : 3);
    double sig = (sigma > 0.0) ? sigma : 1.6;
    double cThr = std::max(0.0, contrast);
    double eThr = std::max(1e-6, edge);

    // Grayscale float base (2× upsampled)
    cv::Mat gray8, gray32, base;
    if (src.channels() == 3) {
        cv::cvtColor(src, gray8, cv::COLOR_BGR2GRAY);
    } else {
        gray8 = src.clone();
    }
    gray8.convertTo(gray32, CV_32F, 1.0 / 255.0);
    cv::resize(gray32, base, {}, 2.0, 2.0, cv::INTER_LINEAR);

    Pyramid gauss = buildGaussianPyramid(base, nOct, nLay, sig);
    Pyramid dog   = buildDoGPyramid(gauss);

    auto kps = detectExtrema(dog, nLay, cThr, eThr, sig, src.rows*2, src.cols*2);
    auto ori = assignOrientations(kps, gauss, nLay);
    if (!kps.empty() && ori.empty()) {
        for (auto& k : kps) {
            k.angle = 0;
        }
    } else {
        kps = std::move(ori);
    }
    kps = filterByResponse(kps, nFeat);

    cv::Mat descs;
    if (!colorMode) {
        descs = computeDescriptors(kps, gauss, nLay);
    } else {
        std::vector<cv::Mat> chs;
        if (src.channels() == 3) {
            cv::split(src, chs);
        } else {
            chs = {src.clone(), src.clone(), src.clone()};
        }

        for (auto& ch : chs) {
            cv::Mat f, b;
            ch.convertTo(f, CV_32F, 1.0 / 255.0);
            cv::resize(f, b, {}, 2.0, 2.0, cv::INTER_LINEAR);
            Pyramid gCh = buildGaussianPyramid(b, nOct, nLay, sig);
            cv::Mat d = computeDescriptors(kps, gCh, nLay);

            if (descs.empty()) {
                descs = d;
            } else {
                cv::Mat merged;
                cv::hconcat(descs, d, merged);
                descs = merged;
            }
        }
    }

    timingMs = std::chrono::duration<double,std::milli>(
        std::chrono::high_resolution_clock::now()-t0).count();

    for (auto& kp : kps) {
        kp.pt.x *= 0.5f;
        kp.pt.y *= 0.5f;
        kp.size *= 0.5f;
    }

    return { drawKeypoints(src, kps), kps, descs };
}