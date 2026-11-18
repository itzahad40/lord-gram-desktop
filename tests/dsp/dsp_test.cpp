#include "FightModeDsp.h"
#include <cstdio>
#include <vector>
#include <cstdlib>
using namespace tgcalls;
static const int SR = 48000, N = 48000;
static std::vector<float> sine(float amp, float f) { std::vector<float> v(N); for (int i = 0; i < N; i++) v[i] = amp * std::sin(2 * 3.14159265f * f * i / SR); return v; }
static double rmsFrom(const std::vector<float>&v,size_t from){double s=0;for(size_t i=from;i<v.size();i++)s+=v[i]*v[i];return std::sqrt(s/(v.size()-from));}
static double rms(const std::vector<float>&v){return rmsFrom(v,N/2);}
static float peak(const std::vector<float>&v){float p=0;for(float x:v)p=std::max(p,std::fabs(x));return p;}
static void run(std::vector<float>&v, FightModeParams p){ FightModeDsp d; for(int i=0;i<N;i+=480) d.process(&v[i],480,SR,p); }
static double db(double r){return 20*std::log10(r);}
static int fails = 0;
#define CHECK(c, msg) do { bool ok=(c); printf("%s %s\n", ok?"PASS":"FAIL", msg); if(!ok) fails++; } while(0)
int main(){
  { auto a=sine(3000,1000), b=a; FightModeParams p; p.enabled=false; p.gainDb=30; run(b,p); CHECK(a==b,"bypass is bit-exact"); }
  // No limiter any more: gain dB should now track accurately even at high gain (nothing bends the curve),
  // but the output WILL go past int16 range (32767) well before 100 dB. This is expected and printed, not hidden.
  for (float g : {0.f, 10.f, 20.f, 32.f, 60.f}) {
    auto a=sine(300,1000), b=a; FightModeParams p; p.enabled=true; p.gainDb=g; run(b,p);
    double d=db(rms(b)/rms(a)); printf("  gain %4.0f dB -> measured %.1f dB (peak %.0f, int16 max is 32767)\n", g, d, peak(b));
    CHECK(std::fabs(d-g)<1.0, "gain dB tracks exactly now (no limiter bending the curve)");
  }
  { auto a=sine(3000,1000), b=a; FightModeParams p; p.enabled=true; p.gainDb=100; run(b,p);
    printf("  normal voice at 100 dB, NO limiter -> peak %.0f (int16 max 32767, ratio %.0fx over)\n", peak(b), peak(b)/32767.f);
    CHECK(peak(b) > 32767.f, "confirms: without the limiter, high gain really does exceed int16 (this is the removed safety net)"); }
  { auto a=sine(15,1000), b=a; FightModeParams p; p.enabled=true; p.gainDb=100; run(b,p); printf("  mic hiss level at 100 dB -> out rms %.0f\n", rms(b)); CHECK(rms(b)<500,"gate keeps mic hiss quiet at 100 dB"); }
  { auto a=sine(15,1000), b=a; FightModeParams p; p.enabled=true; p.gainDb=30; run(b,p); double d=db(rms(b)/rms(a)); CHECK(std::fabs(d-30.0)<1.5,"gate is not active at 30 dB"); }
  { auto a=sine(300,1000), b=a; FightModeParams p; p.enabled=true; p.gainDb=0; p.volume=0.5f; run(b,p); double d=db(rms(b)/rms(a)); printf("  volume 50%% -> %.1f dB\n", d); CHECK(std::fabs(d+6.0)<1.0,"volume 50% is about -6 dB"); }
  { auto a=sine(300,1000), b=a; FightModeParams p; p.enabled=true; p.gainDb=0; p.eqDb[2]=9; run(b,p); double d=db(rms(b)/rms(a)); printf("  EQ 1 kHz +9 dB -> %.1f dB\n", d); CHECK(std::fabs(d-9.0)<1.0,"EQ band at 1 kHz boosts by about +9 dB"); }
  { auto a=sine(300,1000), b=a; FightModeParams p; p.enabled=true; p.gainDb=0; p.eqDb[0]=12; p.eqDb[5]=12; run(b,p); double d=db(rms(b)/rms(a)); CHECK(std::fabs(d)<1.5,"60 Hz / 16 kHz bands do not change 1 kHz much"); }
  { auto a=sine(300,60), b=a; FightModeParams p; p.enabled=true; p.gainDb=0; p.eqDb[0]=12; run(b,p); double d6=db(rms(b)/rms(a)); printf("  EQ 60 Hz +12 dB -> %.1f dB\n", d6); CHECK(d6>8.0,"60 Hz band can really boost bass"); }
  { auto a=sine(3000,1000), b=a; FightModeDsp d; FightModeParams p; p.enabled=true; p.gainDb=20; for(int i=0;i<N/2;i+=480) d.process(&b[i],480,SR,p); p.enabled=false; for(int i=N/2;i<N;i+=480) d.process(&b[i],480,SR,p);
    std::vector<float> t(b.end()-4800,b.end()), ta(a.end()-4800,a.end()); CHECK(std::fabs(db(rmsFrom(t,0)/rmsFrom(ta,0)))<0.2,"turning off fades back to dry"); }
  { FightModeDsp d; FightModeParams p; p.enabled=true; p.gainDb=20; auto a=sine(3000,1000); float lv=0; for(int i=0;i<4800;i+=480) lv=d.process(&a[i],480,SR,p); CHECK(lv>0.05f && lv<=1.0f,"level meter value in range"); }
  // ---- clarity + punch (v4)
  { FightModeParams p; p.enabled=true; p.gainDb=0; p.clarity=1.0f;
    auto a=sine(300,3200), b=a; run(b,p); double d1=db(rms(b)/rms(a));
    auto c=sine(300,250), e=c; run(e,p); double d2=db(rms(e)/rms(c));
    auto f=sine(300,1000), g=f; run(g,p); double d3=db(rms(g)/rms(f));
    printf("  clarity: 3.2 kHz %+.1f dB, 250 Hz %+.1f dB, 1 kHz %+.1f dB\n", d1,d2,d3);
    CHECK(d1>6.0 && d1<11.0,"clarity boosts 3.2 kHz presence by about +8 dB");
    CHECK(d2<-2.0,"clarity cuts 250 Hz mud");
    CHECK(std::fabs(d3)<3.0,"clarity leaves 1 kHz mostly alone"); }
  // compressor removed: "punch" is now inert. Confirm output no longer depends on it at all.
  { FightModeParams pOff; pOff.enabled=true; pOff.gainDb=20; pOff.punch=0.0f;
    FightModeParams pOn = pOff; pOn.punch=1.0f;
    auto a=sine(300,1000), bOff=a, bOn=a; run(bOff,pOff); run(bOn,pOn);
    CHECK(bOff==bOn, "punch no longer changes the output (compressor removed, as requested)"); }
  { FightModeParams p; p.enabled=false; p.punch=1; p.clarity=1; p.gainDb=60; auto a=sine(3000,1000), b=a; run(b,p); CHECK(a==b,"bypass still bit-exact with punch/clarity set"); }
  { FightModeParams p; p.enabled=true; p.gainDb=100; p.punch=1; p.clarity=1; for(int i=0;i<6;i++) p.eqDb[i]=12; p.volume=1;
    auto a=sine(32000,2000), b=a; run(b,p); bool finite=true; for(float x:b) if(std::isnan(x)||std::isinf(x)) finite=false;
    printf("  everything maxed, no limiter -> peak %.0f (int16 max 32767)\n", peak(b));
    CHECK(finite, "numbers stay finite (no NaN/Inf) even unbounded"); }
  { FightModeParams p; p.enabled=true; p.gainDb=30; p.punch=0; p.clarity=0; auto a=sine(300,1000), b=a; run(b,p); double d=db(rms(b)/rms(a)); CHECK(std::fabs(d-30.0)<1.0,"clarity 0 / no punch behaves as plain gain"); }
  // broadcast: de-hiss should reduce background hiss, exciter should not blow the ceiling
  {
    auto a=sine(15,7000), bNoBroadcast=a, bBroadcast=a; // 7 kHz: real hiss lives up here, de-hiss targets 4.5 kHz+
    FightModeDsp d1,d2; FightModeParams p1,p2;
    p1.enabled=true; p1.gainDb=60; p1.clarity=0.9f; p1.punch=0.85f; p1.broadcast=false;
    p2=p1; p2.broadcast=true;
    run2:
    for(int i=0;i<N;i+=480){ d1.process(&bNoBroadcast[i],480,SR,p1); d2.process(&bBroadcast[i],480,SR,p2); }
    double hissOff=rmsFrom(bNoBroadcast,N/2), hissOn=rmsFrom(bBroadcast,N/2);
    printf("  hiss (silence) at 60dB: broadcast OFF %.0f -> broadcast ON %.0f\n", hissOff, hissOn);
    CHECK(hissOn < hissOff, "broadcast reduces hiss during silence (de-hiss)");
  }
  {
    auto a=sine(600,1000), b=a; FightModeDsp d; FightModeParams p; p.enabled=true; p.gainDb=30; p.clarity=0.9f; p.broadcast=true;
    run(b,p);
    printf("  broadcast voice-level peak: %.0f (int16 max 32767, no limiter so this can go over)\n", peak(b));
    bool finite=true; for(float x:b) if(std::isnan(x)||std::isinf(x)) finite=false;
    CHECK(finite, "broadcast stays finite even without the limiter");
  }
  {
    auto a=sine(600,2000), bOff=a, bOn=a; // 2kHz: in the exciter's presence range
    FightModeDsp d1,d2; FightModeParams p1,p2;
    p1.enabled=true; p1.gainDb=20; p1.broadcast=false;
    p2=p1; p2.broadcast=true;
    for(int i=0;i<N;i+=480){ d1.process(&bOff[i],480,SR,p1); d2.process(&bOn[i],480,SR,p2); }
    double rOff=rms(bOff), rOn=rms(bOn);
    printf("  perceived density (2 kHz tone) OFF %.0f -> ON %.0f (%.1f dB)\n", rOff, rOn, db(rOn/rOff));
    CHECK(rOn > rOff, "broadcast adds density/loudness via the exciter");
  }
  printf(fails?"\n%d FAILED\n":"\nALL OK\n", fails); return fails?1:0;
}
