#include "h.h"

B type_c1(B t, B x) {
  i32 r = -1;
       if (isArr(x)) r = 0;
  else if (isI32(x)) r = 1;
  else if (isF64(x)) r = 1;
  else if (isC32(x)) r = 2;
  else if (isFun(x)) r = 3;
  else if (isMd1(x)) r = 4;
  else if (isMd2(x)) r = 5;
  dec(x);
  if (r==-1) return err("getting type");
  return m_i32(r);
}

B decp_c1(B t, B x) {
  if (!isVal(x)) return m_v2(m_i32(-1), x);
  if (v(x)->flags) return m_v2(m_i32(0), x);
  return TI(x).decompose(x);
}

usz runtimeLen;
B primInd_c1(B t, B x) {
  if (!isVal(x)) return m_i32(runtimeLen);
  if (v(x)->flags) return m_i32(v(x)->flags-1);
  return m_i32(runtimeLen);
}

B glyph_c1(B t, B x) {
  return x;
}

B fill_c1(B t,      B x) { dec(x); return m_f64(0); }
B fill_c2(B t, B w, B x) { dec(w); return x; }

B grLen_c1(B t, B x) {
  i64 ria = -1;
  usz ia = a(x)->ia;
  BS2B xget = TI(x).get;
  for (usz i = 0; i < ia; i++) {
    i64 c = o2i64(xget(x, i));
    if (c>ria) ria = c;
  }
  ria++;
  HArr_p r = m_harrv(ria);
  for (usz i = 0; i < ria; i++) r.a[i] = m_f64(0);
  for (usz i = 0; i < ia; i++) {
    i64 n = o2i64(xget(x, i));
    if (n>=0) r.a[n].f++;
  }
  dec(x);
  return r.b;
}
B grLen_c2(B t, B w, B x) {
  i64 ria = o2s(w)-1;
  usz ia = a(x)->ia;
  BS2B xget = TI(x).get;
  for (usz i = 0; i < ia; i++) {
    i64 c = o2i64(xget(x, i));
    if (c>ria) ria = c;
  }
  ria++;
  HArr_p r = m_harrv(ria);
  for (usz i = 0; i < ria; i++) r.a[i] = m_f64(0);
  for (usz i = 0; i < ia; i++) {
    i64 n = o2i64(xget(x, i));
    if (n>=0) r.a[n].f++;
  }
  dec(x);
  return r.b;
}

B grOrd_c2(B t, B w, B x) {
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  if (wia==0) return c1(bi_ud, m_i32(0));
  if (xia==0) return x;
  BS2B wget = TI(w).get;
  BS2B xget = TI(x).get;
  usz tmp[wia];
  tmp[0] = 0;
  for (int i = 1; i < wia; i++) tmp[i] = tmp[i-1]+o2s(wget(w,i-1));
  usz ria = tmp[wia-1]+o2s(wget(w,wia-1));
  HArr_p r = m_harrv(ria);
  for (usz i = 0; i < xia; i++) {
    i64 c = o2i64(xget(x,i));
    if (c>=0) r.a[tmp[c]++] = m_usz(i);
  }
  dec(w);
  dec(x);
  return r.b;
}

B asrt_c1(B t, B x) {
  if (isI32(x) && 1==(i32)x.u) return x;
  if (isF64(x) && 1==x.f) return x;
  dec(x);
  return err("assertion error");
}
B asrt_c2(B t, B w, B x) {
  if (isI32(x) && 1==(i32)x.u) return x;
  if (isF64(x) && 1==x.f) return x;
  dec(x);
  printf("Assertion error: "); fflush(stdout); print(w); printf("\n");
  dec(w);
  return err("assertion error with message");
}


#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = NAME##_c2; c(Fun,bi_##NAME)->c1 = NAME##_c1 ; c(Fun,bi_##NAME)->extra=pf_##NAME;
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = NAME##_c2; c(Fun,bi_##NAME)->c1 = c1_invalid; c(Fun,bi_##NAME)->extra=pf_##NAME;
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = c2_invalid;c(Fun,bi_##NAME)->c1 = NAME##_c1 ; c(Fun,bi_##NAME)->extra=pf_##NAME;

B                   bi_type, bi_decp, bi_primInd, bi_glyph, bi_fill, bi_grLen, bi_grOrd, bi_asrt;
void sysfn_init() { bm(type) bm(decp) bm(primInd) bm(glyph) ba(fill) ba(grLen) bd(grOrd) ba(asrt) }

#undef ba
#undef bd
#undef bm