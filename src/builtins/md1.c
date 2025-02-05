#include "../core.h"
#include "../utils/each.h"
#include "../utils/file.h"
#include "../utils/builtins.h"



static B homFil1(B f, B r, B xf) {
  assert(EACH_FILLS);
  if (isPureFn(f)) {
    if (f.u==bi_eq.u || f.u==bi_ne.u || f.u==bi_feq.u) { dec(xf); return taga(toI32Arr(r)); } // ≠ may return ≥2⋆31, but whatever, this thing is stupid anyway
    if (f.u==bi_fne.u) { dec(xf); return withFill(r, m_harrUv(0).b); }
    if (!noFill(xf)) {
      if (CATCH) { dec(catchMessage); return r; }
      B rf = asFill(c1(f, xf));
      popCatch();
      return withFill(r, rf);
    }
  }
  dec(xf);
  return r;
}
static B homFil2(B f, B r, B wf, B xf) {
  assert(EACH_FILLS);
  if (isPureFn(f)) {
    if (f.u==bi_feq.u || f.u==bi_fne.u) { dec(wf); dec(xf); return taga(toI32Arr(r)); }
    if (!noFill(wf) && !noFill(xf)) {
      if (CATCH) { dec(catchMessage); return r; }
      B rf = asFill(c2(f, wf, xf));
      popCatch();
      return withFill(r, rf);
    }
  }
  dec(wf); dec(xf);
  return r;
}

B tbl_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (!EACH_FILLS) return eachm(f, x);
  B xf = getFillQ(x);
  return homFil1(f, eachm(f, x), xf);
}
B tbl_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  B wf, xf;
  if (EACH_FILLS) wf = getFillQ(w);
  if (EACH_FILLS) xf = getFillQ(x);
  if (isAtm(w)) w = m_atomUnit(w);
  if (isAtm(x)) x = m_atomUnit(x);
  usz wia = a(w)->ia; ur wr = rnk(w);
  usz xia = a(x)->ia; ur xr = rnk(x);
  usz ria = wia*xia;  ur rr = wr+xr;
  if (rr<xr) thrF("⌜: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  
  BS2B wgetU = TI(w,getU);
  BS2B xget = TI(x,get);
  BBB2B fc2 = c2fn(f);
  
  usz ri = 0;
  HArr_p r = m_harrs(ria, &ri);
  for (usz wi = 0; wi < wia; wi++) {
    B cw = wgetU(w,wi);
    for (usz xi = 0; xi < xia; xi++,ri++) {
      r.a[ri] = fc2(f, inc(cw), xget(x,xi));
    }
  }
  usz* rsh = harr_fa(r, rr);
  if (rsh) {
    memcpy(rsh   , a(w)->sh, wr*sizeof(usz));
    memcpy(rsh+wr, a(x)->sh, xr*sizeof(usz));
  }
  dec(w); dec(x);
  if (EACH_FILLS) return homFil2(f, r.b, wf, xf);
  return r.b;
}

B each_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (!EACH_FILLS) return eachm(f, x);
  B xf = getFillQ(x);
  return homFil1(f, eachm(f, x), xf);
}
B each_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if (!EACH_FILLS) return eachd(f, w, x);
  B wf = getFillQ(w);
  B xf = getFillQ(x);
  return homFil2(f, eachd(f, w, x), wf, xf);
}


B scan_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (isAtm(x) || rnk(x)==0) thrM("`: Argument cannot have rank 0");
  ur xr = rnk(x);
  usz ia = a(x)->ia;
  if (ia==0) return x;
  B xf = getFillQ(x);
  if (xr==1 && TI(x,elType)==el_i32 && isFun(f) && v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    i32* xp = i32any_ptr(x);
    if (rtid==0) { // +
      i32* rp; B r = m_i32arrv(&rp, ia);
      i64 c = 0;
      for (usz i = 0; i < ia; i++) {
        rp[i] = c+= xp[i];
        if (c>I32_MAX) { dec(r); goto base; }
      }
      dec(x);
      return r;
    }
    if (rtid==7) { // ⌈
      i32* rp; B r = m_i32arrv(&rp, ia);
      i32 c = I32_MIN;
      for (usz i = 0; i < ia; i++) {
        if (xp[i]>c) c = xp[i];
        rp[i] = c;
      }
      dec(x);
      return r;
    }
    if (rtid==14) { // ≠
      i32* rp; B r = m_i32arrv(&rp, ia);
      i32 c = 0;
      for (usz i = 0; i < ia; i++) rp[i] = c = c!=xp[i];
      dec(x);
      return r;
    }
  }
  base:;
  
  bool reuse = v(x)->type==t_harr && reusable(x);
  usz i = 0;
  HArr_p r = reuse? harr_parts(x) : m_harrs(a(x)->ia, &i);
  BS2B xget = reuse? TI(x,getU) : TI(x,get);
  BBB2B fc2 = c2fn(f);
  
  if (xr==1) {
    r.a[i] = xget(x,0); i++;
    for (i = 1; i < ia; i++) r.a[i] = fc2(f, inc(r.a[i-1]), xget(x,i));
  } else {
    usz csz = arr_csz(x);
    for (; i < csz; i++) r.a[i] = xget(x,i);
    for (; i < ia; i++) r.a[i] = fc2(f, inc(r.a[i-csz]), xget(x,i));
  }
  return withFill(reuse? x : harr_fcd(r, x), xf);
}
B scan_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if (isAtm(x) || rnk(x)==0) thrM("`: 𝕩 cannot have rank 0");
  ur xr = rnk(x); usz* xsh = a(x)->sh; usz ia = a(x)->ia;
  B wf = getFillQ(w);
  bool reuse = (v(x)->type==t_harr && reusable(x)) | !ia;
  usz i = 0;
  if (xr==1 && q_i32(w) && TI(x,elType)==el_i32 && isFun(f) && v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    i32* xp = i32any_ptr(x);
    i32 wv = o2iu(w);
    if (rtid==0) { // +
      i32* rp; B r = m_i32arrv(&rp, ia);
      i64 c = wv;
      for (usz i = 0; i < ia; i++) {
        rp[i] = c+= xp[i];
        if (c>I32_MAX) { dec(r); goto base; }
      }
      dec(x);
      return r;
    }
    if (rtid==7) { // ⌈
      i32* rp; B r = m_i32arrv(&rp, ia);
      i32 c = wv;
      for (usz i = 0; i < ia; i++) {
        if (xp[i]>c) c = xp[i];
        rp[i] = c;
      }
      dec(x);
      return r;
    }
    if (rtid==14) { // ≠
      i32* rp; B r = m_i32arrv(&rp, ia);
      i32 c = wv;
      for (usz i = 0; i < ia; i++) rp[i] = c = c!=xp[i];
      dec(x);
      return r;
    }
  }
  base:;
  
  HArr_p r = reuse? harr_parts(x) : m_harrs(a(x)->ia, &i);
  BS2B xget = reuse? TI(x,getU) : TI(x,get);
  BBB2B fc2 = c2fn(f);
  
  if (isArr(w)) {
    ur wr = rnk(w); usz* wsh = a(w)->sh; BS2B wget = TI(w,get);
    if (wr+1!=xr || !eqShPrefix(wsh, xsh+1, wr)) thrF("`: Shape of 𝕨 must match the cell of 𝕩 (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
    if (ia==0) return x;
    usz csz = arr_csz(x);
    for (; i < csz; i++) r.a[i] = fc2(f, wget(w,i), xget(x,i));
    for (; i < ia; i++) r.a[i] = fc2(f, inc(r.a[i-csz]), xget(x,i));
    dec(w);
  } else {
    if (xr!=1) thrF("`: Shape of 𝕨 must match the cell of 𝕩 (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
    if (ia==0) return x;
    B pr = r.a[0] = fc2(f, w, xget(x,0)); i++;
    for (; i < ia; i++) r.a[i] = pr = fc2(f, inc(pr), xget(x,i));
  }
  return withFill(reuse? x : harr_fcd(r, x), wf);
}

B fold_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (isAtm(x) || rnk(x)!=1) thrF("´: Argument must be a list (%H ≡ ≢𝕩)", x);
  usz ia = a(x)->ia;
  if (TI(x,elType)==el_i32 && isFun(f) && v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    i32* xp = i32any_ptr(x);
    if (rtid==0) { // +
      i64 c = 0;
      for (usz i = 0; i < ia; i++) if ((c+= xp[i]) > I32_MAX) goto base;
      dec(x); return m_i32(c);
    }
    if (rtid==2 | rtid==10) { // ×/∧
      i64 c = 1;
      for (usz i = 0; i < ia; i++) if ((c*= xp[i]) > I32_MAX) goto base;
      dec(x); return m_i32(c);
    }
    if (rtid==6) { // ⌊
      if (ia==0) return m_f64(1.0/0.0);
      i32 c = I32_MAX;
      for (usz i = 0; i < ia; i++) {
        if (xp[i]<c) c = xp[i];
      }
      dec(x); return m_i32(c);
    }
    if (rtid==7) { // ⌈
      if (ia==0) return m_f64(-1.0/0.0);
      i32 c = I32_MIN;
      for (usz i = 0; i < ia; i++) {
        if (xp[i]>c) c = xp[i];
      }
      dec(x); return m_i32(c);
    }
    if (rtid==11) { // ∨
      bool any = false;
      for (usz i = 0; i < ia; i++) {
        i32 c = xp[i];
        if (c!=0 && c!=1) goto base;
        any|= c;
      }
      dec(x); return m_i32(any);
    }
  }
  if (ia==0) {
    dec(x);
    if (isFun(f)) {
      B r = TI(f,identity)(f);
      if (!q_N(r)) return inc(r);
    }
    thrM("´: No identity found");
  }
  base:;
  
  BS2B xget = TI(x,get);
  BBB2B fc2 = c2fn(f);
  B c;
  if (TI(x,elType)==el_i32) {
    i32* xp = i32any_ptr(x);
    c = m_i32(xp[ia-1]);
    for (usz i = ia-1; i>0; i--) c = fc2(f, m_i32(xp[i-1]), c);
  } else {
    c = xget(x, ia-1);
    for (usz i = ia-1; i>0; i--) c = fc2(f, xget(x, i-1), c);
  }
  dec(x);
  return c;
}
B fold_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if (isAtm(x) || rnk(x)!=1) thrF("´: 𝕩 must be a list (%H ≡ ≢𝕩)", x);
  usz ia = a(x)->ia;
  if (q_i32(w) && TI(x,elType)==el_i32 && isFun(f) && v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    i32* xp = i32any_ptr(x);
    i32 wv = o2iu(w);
    i64 c = wv;
    if (rtid==0) { // +
      for (usz i = 0; i < ia; i++) if ((c+= xp[i]) > I32_MAX) goto base;
      dec(x); return m_i32(c);
    }
    if (rtid==2 | rtid==10) { // ×/∧
      for (usz i = 0; i < ia; i++) if ((c*= xp[i]) > I32_MAX) goto base;
      dec(x); return m_i32(c);
    }
    if (rtid==6) { // ⌊
      for (usz i = 0; i < ia; i++) if (xp[i]<c) c = xp[i];
      dec(x); return m_i32(c);
    }
    if (rtid==7) { // ⌈
      for (usz i = 0; i < ia; i++) if (xp[i]>c) c = xp[i];
      dec(x); return m_i32(c);
    }
  }
  base:;
  
  B c = w;
  BS2B xget = TI(x,get);
  BBB2B fc2 = c2fn(f);
  for (usz i = ia; i>0; i--) c = fc2(f, xget(x, i-1), c);
  dec(x);
  return c;
}

B const_c1(B d     , B x) {         dec(x); return inc(c(Md1D,d)->f); }
B const_c2(B d, B w, B x) { dec(w); dec(x); return inc(c(Md1D,d)->f); }

B swap_c1(B d     , B x) { return c2(c(Md1D,d)->f, inc(x), x); }
B swap_c2(B d, B w, B x) { return c2(c(Md1D,d)->f,     x , w); }


B timed_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  i64 am = o2i64(w);
  for (i64 i = 0; i < am; i++) inc(x);
  dec(x);
  u64 sns = nsTime();
  for (i64 i = 0; i < am; i++) dec(c1(f, x));
  u64 ens = nsTime();
  return m_f64((ens-sns)/(1e9*am));
}
B timed_c1(B d, B x) { B f = c(Md1D,d)->f;
  u64 sns = nsTime();
  dec(c1(f, x));
  u64 ens = nsTime();
  return m_f64((ens-sns)*1e-9);
}


extern B rt_cell;
B cell_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (isAtm(x) || rnk(x)==0) {
    B r = c1(f, x);
    return isAtm(r)? m_atomUnit(r) : r;
  }
  // if (f.u == bi_lt.u) return toCells(x); // TODO fix fills
  usz cr = rnk(x)-1;
  usz cam = a(x)->sh[0];
  usz csz = arr_csz(x);
  ShArr* csh;
  if (cr>1) {
    csh = m_shArr(cr);
    memcpy(csh->a, a(x)->sh+1, sizeof(usz)*cr);
  }
  usz i = 0;
  BSS2A slice = TI(x,slice);
  HArr_p r = m_harrs(cam, &i);
  usz p = 0;
  for (; i < cam; i++) {
    Arr* s = slice(inc(x), p, csz); arr_shSetI(s, cr, csh);
    r.a[i] = c1(f, taga(s));
    p+= csz;
  }
  if (cr>1) ptr_dec(csh);
  dec(x);
  return bqn_merge(harr_fv(r));
}
B cell_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if ((isAtm(x) || rnk(x)==0) && (isAtm(w) || rnk(w)==0)) {
    B r = c2(f, w, x);
    return isAtm(r)? m_atomUnit(r) : r;
  }
  B fn = m1_d(inc(rt_cell), inc(f)); // TODO
  B r = c2(fn, w, x);
  dec(fn);
  return r;
}



static void print_md1BI(B x) { printf("%s", format_pm1(c(Md1,x)->extra)); }
void md1_init() {
  TIi(t_md1BI,print) = print_md1BI;
}
