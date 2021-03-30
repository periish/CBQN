#include "h.h"

enum {
  PUSH =  0, // N; push object from objs[N]
  VARO =  1, // N; push variable with name strs[N]
  VARM =  2, // N; push mutable variable with name strs[N]
  ARRO =  3, // N; create a vector of top N items
  ARRM =  4, // N; create a mutable vector of top N items
  FN1C =  5, // monadic function call ⟨…,x,f  ⟩ → F x
  FN2C =  6, //  dyadic function call ⟨…,x,f,w⟩ → w F x
  OP1D =  7, // derive 1-modifier to function; ⟨…,  _m,f⟩ → (f _m)
  OP2D =  8, // derive 2-modifier to function; ⟨…,g,_m,f⟩ → (f _m_ g)
  TR2D =  9, // derive 2-train aka atop; ⟨…,  g,f⟩ → (f g)
  TR3D = 10, // derive 3-train aka fork; ⟨…,h,g,f⟩ → (f g h)
  SETN = 11, // set new; _  ←_; ⟨…,x,  mut⟩ → mut←x
  SETU = 12, // set upd; _  ↩_; ⟨…,x,  mut⟩ → mut↩x
  SETM = 13, // set mod; _ F↩_; ⟨…,x,F,mut⟩ → mut F↩x
  POPS = 14, // pop object from stack
  DFND = 15, // N; push dfns[N], derived to current scope
  FN1O = 16, // optional monadic call (FN1C but checks for · at 𝕩)
  FN2O = 17, // optional  dyadic call (FN2C but checks for · at 𝕩 & 𝕨)
  CHKV = 18, // throw error if top of stack is ·
  TR3O = 19, // TR3D but creates an atop if F is ·
  OP2H = 20, // derive 2-modifier to 1-modifier ⟨…,g,_m_⟩ → (_m_ g)
  LOCO = 21, // N0,N1; push variable at depth N0 and position N1
  LOCM = 22, // N0,N1; push mutable variable at depth N0 and position N1
  VFYM = 23, // push a mutable version of ToS that fails if set to a non-equal value (for header assignment)
  SETH = 24, // set header; acts like SETN, but it doesn't push to stack, and, instead of erroring in cases it would, it skips to the next body
  RETN = 25, // returns top of stack
  FLDO = 26, // N; get field objs[N] of ToS
  FLDM = 27, // N; set field objs[N] from ToS
  NSPM = 28, // N0,N1; create a destructible namespace from top N0 items, with the keys objs[N1]
  RETD = 29, // return a namespace of exported items
  SYSV = 30, // N; get system function N
};

i32* nextBC(i32* p) {
  switch(*p) {
    case PUSH: case DFND: case ARRO: case ARRM:
    case VARO: case VARM: case FLDO: case FLDM:
    case SYSV:
      return p+2;
    case FN1C: case FN2C: case FN1O: case FN2O:
    case OP1D: case OP2D: case OP2H:
    case TR2D: case TR3D: case TR3O:
    case SETN: case SETU: case SETM: case SETH:
    case POPS: case CHKV: case VFYM: case RETN: case RETD:
      return p+1;
    case LOCO: case LOCM: case NSPM:
      return p+3;
    default: return 0;
  }
}
i32 stackDiff(i32* p) {
  switch(*p) {
    case PUSH: case VARO: case VARM: case DFND: case LOCO: case LOCM: case SYSV:
      return 1;
    case CHKV: case VFYM: case FLDO: case FLDM: case RETD:
      return 0;
    case FN1C: case OP1D: case TR2D: case SETN: case SETU: case POPS: case FN1O: case OP2H: case SETH: case RETN:
      return -1;
    case FN2C: case OP2D: case TR3D: case SETM: case FN2O: case TR3O:
      return -2;
    case ARRO: case ARRM: case NSPM:
      return 1-p[1];
    default: return 9999999;
  }
}
char* nameBC(i32* p) {
  switch(*p) { default: return "(unknown)";
    case PUSH:return "PUSH";case VARO:return "VARO";case VARM:return "VARM";case ARRO:return "ARRO";
    case ARRM:return "ARRM";case FN1C:return "FN1C";case FN2C:return "FN2C";case OP1D:return "OP1D";
    case OP2D:return "OP2D";case TR2D:return "TR2D";case TR3D:return "TR3D";case SETN:return "SETN";
    case SETU:return "SETU";case SETM:return "SETM";case POPS:return "POPS";case DFND:return "DFND";
    case FN1O:return "FN1O";case FN2O:return "FN2O";case CHKV:return "CHKV";case TR3O:return "TR3O";
    case OP2H:return "OP2H";case LOCO:return "LOCO";case LOCM:return "LOCM";case VFYM:return "VFYM";
    case SETH:return "SETH";case RETN:return "RETN";case FLDO:return "FLDO";case FLDM:return "FLDM";
    case NSPM:return "NSPM";case RETD:return "RETD";case SYSV:return "SYSV";
  }
}
void printBC(i32* p) {
  printf("%s", nameBC(p));
  i32* n = nextBC(p);
  p++;
  i32 am = n-p;
  i32 len = 0;
  for (i32 i = 0; i < am; i++) printf(" %d", p[i]);
  while(p!=n) {
    i32 c = *p++;
    i32 pow = 10;
    i32 log = 1;
    while (pow<=c) { pow*=10; log++; }
    len+= log+1;
  }
  len = 6-len;
  while(len-->0) printf(" ");
}

typedef struct Block Block;
typedef struct Body Body;
typedef struct Scope Scope;

typedef struct Comp {
  struct Value;
  B bc;
  HArr* objs;
  Block* blocks[];
} Comp;

typedef struct Block {
  struct Value;
  bool imm;
  u8 ty;
  Body* body;
} Block;

typedef struct Body {
  struct Value;
  Comp* comp;
  // B* objs;
  i32* bc; // pointer in comp->bc
  u32 maxStack;
  u16 varAm;
  // HArr* vNames;
} Body;

typedef struct Scope {
  struct Value;
  u64 bcInd; // DEBUG: place of this in bytecode array
  u16 varAm;
  Scope* psc;
  B vars[];
} Scope;

typedef struct FunBlock { struct Fun; Scope* sc; Block* bl; } FunBlock;
typedef struct Md1Block { struct Md1; Scope* sc; Block* bl; } Md1Block;
typedef struct Md2Block { struct Md2; Scope* sc; Block* bl; } Md2Block;


Block* compile(B bcq, B objs, B blocks) {
  usz bam = a(blocks)->ia;
  
  I32Arr* bca = toI32Arr(bcq);
  i32* bc = bca->a;
  usz bcl = bca->ia;
  Comp* comp = mm_allocN(fsizeof(Comp,blocks,Block*,bam), t_comp);
  comp->bc = tag(bca, ARR_TAG);
  comp->objs = toHArr(objs);
  B* blockDefs = toHArr(blocks)->a;
  
  for (usz i = 0; i < bam; i++) {
    B cbld = blockDefs[i];
    if (a(cbld)->ia != 4) return c(Block,err("bad compile block")); // todo not cast errors here weirdly
    BS2B bget = TI(cbld).get;
    usz  ty  = o2s(bget(cbld,0)); if (ty<0|ty>2) return c(Block,err("bad block type"));
    bool imm = o2s(bget(cbld,1)); // todo o2b or something
    usz  idx = o2s(bget(cbld,2)); if (idx>=bcl) return c(Block,err("oob bytecode index"));
    usz  vam = o2s(bget(cbld,3));
    i32* cbc = bc+idx;
    
    i32* scan = cbc;
    i32 ssz = 0, mssz=0;
    bool needsV0 = false;
    while (*scan!=RETN & *scan!=RETD) {
      ssz+= stackDiff(scan);
      if (ssz>mssz) mssz = ssz;
      if (*scan==LOCO|*scan==LOCM) { i32 d=scan[1]; i32 p=scan[2];
        if (d>U16_MAX) return c(Block,err("LOC_ too deep"));
        if (d==0 & p==0) needsV0 = true;
      }
      scan = nextBC(scan);
      if (scan-bc >= bcl) return c(Block,err("no RETN/RETD found at end of bytecode"));
    }
    
    Body* body = mm_allocN(sizeof(Body), t_body);
    body->comp = comp;
    body->bc = cbc;
    body->maxStack = mssz;
    body->varAm = vam;
    if (needsV0) body->flags|= 1;
    
    Block* bl = mm_allocN(sizeof(Block), t_block);
    bl->body = body;
    bl->imm = imm;
    bl->ty = ty;
    comp->blocks[i] = bl;
  }
  
  Block* ret = c(Block,inci(tag(comp->blocks[0], OBJ_TAG)));
  // TODO store blocks in each body, then decrement each of comp->blocks; also then move temp block list out of Comp as it's useless then
  // for (usz i = 0; i < bam; i++) ptr_dec(comp->blocks[i]);
  
  dec(blocks);
  return ret;
}

Scope* scd(Scope* sc, u16 d) {
  for (i32 i = 0; i < d; i++) sc = sc->psc;
  return sc;
}

B v_set(Scope* sc, B s, B x, bool upd) { // frees s, consumes x, returns previous value
  if (isVar(s)) {
    sc = scd(sc, (u16)(s.u>>32));
    B prev = sc->vars[(u32)s.u];
    if (upd) {
      if (prev.u==bi_noVar.u) return err("updating undefined variable");
      dec(prev);
    } else {
      if (prev.u!=bi_noVar.u) return err("redefining variable");
    }
    sc->vars[(u32)s.u] = x;
  } else {
    VT(s, t_harr);
    if (!shEq(s, x)) return err("spread assignment: mismatched shape");
    usz ia = a(x)->ia;
    B* sp = harr_ptr(s);
    BS2B xget = TI(x).get;
    for (u64 i = 0; i < ia; i++) v_set(sc, sp[i], xget(x,i), upd);
    dec(s); dec(x);
  }
  return m_f64(0);
}
B v_get(Scope* sc, B s) { // get value representing s, replacing with bi_optOut; doesn't consume
  if (isVar(s)) {
    sc = scd(sc, (u16)(s.u>>32));
    B r = sc->vars[(u32)s.u];
    sc->vars[(u32)s.u] = bi_optOut;
    return r;
  } else {
    VT(s, t_harr);
    usz ia = a(s)->ia;
    B* sp = harr_ptr(s);
    HArr_p r = m_harrv(ia);
    for (u64 i = 0; i < ia; i++) r.a[i] = v_get(sc, sp[i]);
    return r.b;
  }
}

// none consume anything consume
B m_funBlock(Block* bl, Scope* psc); // may return evaluated result, whatever
B m_md1Block(Block* bl, Scope* psc);
B m_md2Block(Block* bl, Scope* psc);
#ifdef DEBUG_VM
i32 bcDepth=-2;
i32* vmStack;
i32 bcCtr = 0;
#endif
B evalBC(Body* b, Scope* sc) {
  #ifdef DEBUG_VM
    bcDepth+= 2;
    if (!vmStack) vmStack = malloc(400);
    i32 stackNum = bcDepth>>1;
    vmStack[stackNum] = -1;
    printf("new eval\n");
  #endif
  B* objs = b->comp->objs->a;
  Block** blocks = b->comp->blocks;
  i32* bc = b->bc;
  B stack[b->maxStack];
  i32 sh = 0;
  #define POP stack[--sh]
  #define P(N) B N=POP;
  #define ADD stack[sh++] = 
  while(true) {
    #ifdef DEBUG_VM
      i32* sbc = bc;
      i32 bcPos = sbc-c(I32Arr,b->comp->bc)->a;
      vmStack[stackNum] = bcPos;
      for(i32 i = 0; i < bcDepth; i++) printf(" ");
      printBC(sbc); printf("@%d << ", bcPos);
      for (i32 i = 0; i < sh; i++) { if(i)printf(" ⋄ "); print(stack[i]); } puts(""); fflush(stdout);
      bcCtr++;
      for (i32 i = 0; i < sc->varAm; i++) validate(sc->vars[i]);
    #endif
    switch(*bc++) {
      case POPS: dec(POP); break;
      case PUSH: {
        ADD inci(objs[*bc++]);
        break;
      }
      case FN1C: { P(f)P(x)
        ADD c1(f, x); dec(f);
        break;
      }
      case FN1O: { P(f)P(x)
        ADD isNothing(x)? x : c1(f, x); dec(f);
        break;
      }
      case FN2C: { P(w)P(f)P(x)
        ADD c2(f, w, x); dec(f);
        break;
      }
      case FN2O: { P(w)P(f)P(x)
        if (isNothing(x)) { dec(w); ADD x; }
        else ADD isNothing(w)? c1(f, x) : c2(f, w, x);
        dec(f);
        break;
      }
      case ARRO: case ARRM: {
        i32 sz = *bc++;
        HArr_p r = m_harrv(sz);
        for (i32 i = 0; i < sz; i++) r.a[sz-i-1] = POP;
        ADD r.b;
        break;
      }
      case DFND: {
        Block* bl = blocks[*bc++];
        switch(bl->ty) { default: UD;
          case 0: ADD m_funBlock(bl, sc); break;
          case 1: ADD m_md1Block(bl, sc); break;
          case 2: ADD m_md2Block(bl, sc); break;
        }
        break;
      }
      case OP1D: { P(f)P(m)     ADD m1_d  (m,f  ); break; }
      case OP2D: { P(f)P(m)P(g) ADD m2_d  (m,f,g); break; }
      case OP2H: {     P(m)P(g) ADD m2_h  (m,  g); break; }
      case TR2D: {     P(g)P(h) ADD m_atop(  g,h); break; }
      case TR3D: { P(f)P(g)P(h) ADD m_fork(f,g,h); break; }
      case TR3O: { P(f)P(g)P(h)
        if (isNothing(f)) { ADD m_atop(g,h); dec(f); }
        else ADD m_fork(f,g,h);
        break;
      }
      case LOCM: { i32 d = *bc++; i32 p = *bc++;
        ADD tag((u64)d<<32 | p, VAR_TAG);
        break;
      }
      case LOCO: { i32 d = *bc++; i32 p = *bc++;
        ADD inci(scd(sc,d)->vars[p]);
        break;
      }
      case SETN: { P(s)    P(x) v_set(sc, s, inci(x), false); ADD x; break; }
      case SETU: { P(s)    P(x) v_set(sc, s, inci(x), true ); ADD x; break; }
      case SETM: { P(s)P(f)P(x)
        B w = v_get(sc, s);
        B r = c2(f,w,x); dec(f);
        v_set(sc, s, inci(r), true);
        ADD r;
        break;
      }
      // not implemented: VARO VARM CHKV VFYM SETH FLDO FLDM NSPM RETD SYSV
      default: printf("todo %d\n", bc[-1]); bc++; break;
      case RETN: goto end;
    }
    #ifdef DEBUG_VM
      for(i32 i = 0; i < bcDepth; i++) printf(" ");
      printBC(sbc); printf("@%ld:   ", sbc-c(I32Arr,b->comp->bc)->a);
      for (i32 i = 0; i < sh; i++) { if(i)printf(" ⋄ "); print(stack[i]); } puts(""); fflush(stdout);
    #endif
  }
  #undef P
  #undef ADD
  #undef POP
  end:;
  #ifdef DEBUG_VM
    bcDepth-= 2;
  #endif
  assert(sh==1);
  return stack[0];
}

B actualExec(Block* bl, Scope* psc, u32 ga, ...) {
  Body* bdy = bl->body;
  Scope* sc = mm_allocN(fsizeof(Scope, vars, B, bdy->varAm), t_scope);
  sc->psc = psc; if(psc)ptr_inc(psc);
  u16 varAm = sc->varAm = bdy->varAm;
  assert(varAm>=ga);
  sc->bcInd = bdy->bc-c(I32Arr,bdy->comp->bc)->a;
  i32 i = 0;
  va_list vargs;
  va_start(vargs, ga);
  while (i<ga) sc->vars[i++] = va_arg(vargs, B);
  while (i<varAm) sc->vars[i++] = bi_noVar;
  va_end(vargs);
  // if (ga & !(bdy->flags&1)) {
  //   B v0 = sc->vars[0];
  //   dec(v0); sc->vars[0] = bi_optOut; // prevent reference cycle; TODO more properly do this (or just remove, it doesn't seem to be doing much)
  // }
  B r = evalBC(bdy, sc);
  ptr_dec(sc);
  return r;
}

B funBl_c1(B t,                B x) { return actualExec(c(FunBlock, t)->bl, c(FunBlock, t)->sc, 3, inci(t),                         x, bi_nothing);                            }
B funBl_c2(B t,           B w, B x) { return actualExec(c(FunBlock, t)->bl, c(FunBlock, t)->sc, 3, inci(t),                         x, w);                                     }
B md1Bl_c1(B t, B f,           B x) { return actualExec(c(Md1Block, t)->bl, c(Md1Block, t)->sc, 5, m_md1D(inci(t),inci(f)        ), x, bi_nothing, inci(t), inci(f));          }
B md1Bl_c2(B t, B f,      B w, B x) { return actualExec(c(Md1Block, t)->bl, c(Md1Block, t)->sc, 5, m_md1D(inci(t),inci(f)        ), x, w         , inci(t), inci(f));          }
B md2Bl_c1(B t, B f, B g,      B x) { return actualExec(c(Md2Block, t)->bl, c(Md2Block, t)->sc, 6, m_md2D(inci(t),inci(f),inci(g)), x, bi_nothing, inci(t), inci(f), inci(g)); }
B md2Bl_c2(B t, B f, B g, B w, B x) { return actualExec(c(Md2Block, t)->bl, c(Md2Block, t)->sc, 6, m_md2D(inci(t),inci(f),inci(g)), x, w         , inci(t), inci(f), inci(g)); }
B m_funBlock(Block* bl, Scope* psc) {
  if (bl->imm) return actualExec(bl, psc, 0);
  B r = mm_alloc(sizeof(FunBlock), t_fun_block, ftag(FUN_TAG));
  c(FunBlock, r)->bl = bl; ptr_inc(bl);
  c(FunBlock, r)->sc = psc; ptr_inc(psc);
  c(FunBlock, r)->c1 = funBl_c1;
  c(FunBlock, r)->c2 = funBl_c2;
  return r;
}
B m_md1Block(Block* bl, Scope* psc) {
  B r = mm_alloc(sizeof(Md1Block), t_md1_block, ftag(MD1_TAG));
  c(Md1Block, r)->bl = bl; ptr_inc(bl);
  c(Md1Block, r)->sc = psc; ptr_inc(psc);
  c(Md1Block, r)->c1 = md1Bl_c1;
  c(Md1Block, r)->c2 = md1Bl_c2;
  return r;
}
B m_md2Block(Block* bl, Scope* psc) {
  B r = mm_alloc(sizeof(Md2Block), t_md2_block, ftag(MD2_TAG));
  c(Md2Block, r)->bl = bl; ptr_inc(bl);
  c(Md2Block, r)->sc = psc; ptr_inc(psc);
  c(Md2Block, r)->c1 = md2Bl_c1;
  c(Md2Block, r)->c2 = md2Bl_c2;
  return r;
}

void  comp_free(B x) { Comp*  c = c(Comp ,x); ptr_dec(c->objs); dec(c->bc); }
void  body_free(B x) { Body*  c = c(Body ,x); ptr_dec(c->comp); }
void block_free(B x) { Block* c = c(Block,x); ptr_dec(c->body); }
void scope_free(B x) {
  Scope* c = c(Scope,x);
  if (c->psc) ptr_dec(c->psc);
  u16 am = c->varAm;
  for (u32 i = 0; i < am; i++) dec(c->vars[i]);
}
void funBl_free(B x) { FunBlock* c = c(FunBlock,x); ptr_dec(c->sc); ptr_dec(c->bl); }
void md1Bl_free(B x) { Md1Block* c = c(Md1Block,x); ptr_dec(c->sc); ptr_dec(c->bl); }
void md2Bl_free(B x) { Md2Block* c = c(Md2Block,x); ptr_dec(c->sc); ptr_dec(c->bl); }

void comp_print (B x) { printf("(%p: comp)",v(x)); }
void body_print (B x) { printf("(%p: body varam=%d)",v(x),c(Body,x)->varAm); }
void block_print(B x) { printf("(%p: block for %p)",v(x),c(Block,x)->body); }
void scope_print(B x) { printf("(%p: scope; vars:",v(x));Scope*sc=c(Scope,x);for(u64 i=0;i<sc->varAm;i++){printf(" ");print(sc->vars[i]);}printf(")"); }

// void funBl_print(B x) { printf("(%p: function"" block bl=%p sc=%p)",v(x),c(FunBlock,x)->bl,c(FunBlock,x)->sc); }
// void md1Bl_print(B x) { printf("(%p: 1-modifier block bl=%p sc=%p)",v(x),c(Md1Block,x)->bl,c(Md1Block,x)->sc); }
// void md2Bl_print(B x) { printf("(%p: 2-modifier block bl=%p sc=%p)",v(x),c(Md2Block,x)->bl,c(Md2Block,x)->sc); }
// void funBl_print(B x) { printf("(%p: function"" block @%ld)",v(x),c(FunBlock,x)->bl->body->bc-c(I32Arr,c(FunBlock,x)->bl->body->comp->bc)->a); }
// void md1Bl_print(B x) { printf("(%p: 1-modifier block @%ld)",v(x),c(Md1Block,x)->bl->body->bc-c(I32Arr,c(Md1Block,x)->bl->body->comp->bc)->a); }
// void md2Bl_print(B x) { printf("(%p: 2-modifier block @%ld)",v(x),c(Md2Block,x)->bl->body->bc-c(I32Arr,c(Md2Block,x)->bl->body->comp->bc)->a); }
void funBl_print(B x) { printf("{function""}"); }
void md1Bl_print(B x) { printf("{1-modifier}"); }
void md2Bl_print(B x) { printf("{2-modifier}"); }

B block_decompose(B x) { return m_v2(m_i32(1), x); }

B bl_m1d(B m, B f     ) { Md1Block* c = c(Md1Block,m); return c->bl->imm? actualExec(c(Md1Block, m)->bl, c(Md1Block, m)->sc, 2, m, f   ) : m_md1D(m,f  ); }
B bl_m2d(B m, B f, B g) { Md2Block* c = c(Md2Block,m); return c->bl->imm? actualExec(c(Md2Block, m)->bl, c(Md2Block, m)->sc, 3, m, f, g) : m_md2D(m,f,g); }

void comp_init() {
  ti[t_comp     ].free = comp_free;  ti[t_comp     ].print =  comp_print;
  ti[t_body     ].free = body_free;  ti[t_body     ].print =  body_print;
  ti[t_block    ].free = block_free; ti[t_block    ].print = block_print;
  ti[t_scope    ].free = scope_free; ti[t_scope    ].print = scope_print;
  ti[t_fun_block].free = funBl_free; ti[t_fun_block].print = funBl_print; ti[t_fun_block].decompose = block_decompose;
  ti[t_md1_block].free = md1Bl_free; ti[t_md1_block].print = md1Bl_print; ti[t_md1_block].decompose = block_decompose; ti[t_md1_block].m1_d=bl_m1d;
  ti[t_md2_block].free = md2Bl_free; ti[t_md2_block].print = md2Bl_print; ti[t_md2_block].decompose = block_decompose; ti[t_md2_block].m2_d=bl_m2d;
}

void print_vmStack() {
  #ifdef DEBUG_VM
    printf("vm stack:");
    for (i32 i = 0; i < (bcDepth>>1) + 1; i++) { printf(" %d", vmStack[i]); fflush(stdout); }
    printf("\n"); fflush(stdout);
  #endif
}