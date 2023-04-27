// Copyright (C) 2021-2023 Hibiki AI Limited <info@hibiki-ai.com>
//
// This file is part of ichimoku.
//
// ichimoku is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// ichimoku is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// ichimoku. If not, see <https://www.gnu.org/licenses/>.

// ichimoku - Functions Utilising R's C API ------------------------------------

#define R_NO_REMAP
#define STRICT_R_HEADERS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Visibility.h>

SEXP xts_IndexSymbol;
SEXP xts_IndexTclassSymbol;
SEXP xts_IndexTzoneSymbol;
SEXP ichimoku_PeriodsSymbol;
SEXP ichimoku_PeriodicitySymbol;
SEXP ichimoku_TickerSymbol;

SEXP ichimoku_klass;
SEXP ichimoku_tclass;
SEXP ichimoku_tzone;

typedef SEXP (*c_fun) (SEXP);

// rolling max over a window
SEXP _wmax(const SEXP x, const SEXP window) {

  const double *px = REAL(x);
  const R_xlen_t n = Rf_xlength(x);
  const int w = INTEGER(window)[0], w1 = w - 1;

  SEXP vec = Rf_allocVector(REALSXP, n);
  double *pvec = REAL(vec), s = 0;

  for (int i = 0; i < w1; i++) {
    pvec[i] = NA_REAL;
  }
  for (R_xlen_t i = w1; i < n; i++) {
    s = px[i];
    for (int j = 1; j < w; j++) {
      if (px[i - j] > s)
        s = px[i - j];
    }
    pvec[i] = s;
  }

  return vec;

}

// rolling min over a window
SEXP _wmin(const SEXP x, const SEXP window) {

  const double *px = REAL(x);
  const R_xlen_t n = Rf_xlength(x);
  const int w = INTEGER(window)[0], w1 = w - 1;

  SEXP vec = Rf_allocVector(REALSXP, n);
  double *pvec = REAL(vec), s = 0;

  for (int i = 0; i < w1; i++) {
    pvec[i] = NA_REAL;
  }
  for (R_xlen_t i = w1; i < n; i++) {
    s = px[i];
    for (int j = 1; j < w; j++) {
      if (px[i - j] < s)
        s = px[i - j];
    }
    pvec[i] = s;
  }

  return vec;

}

// rolling mean over a window
SEXP _wmean(const SEXP x, const SEXP window) {

  const double *px = REAL(x);
  const R_xlen_t n = Rf_xlength(x);
  const int w = INTEGER(window)[0], w1 = w - 1;

  SEXP vec = Rf_allocVector(REALSXP, n);
  double *pvec = REAL(vec);
  long double s = 0;

  for (int i = 0; i < w1; i++) {
    s += px[i];
    pvec[i] = NA_REAL;
  }
  for (R_xlen_t i = w1; i < n; i++) {
    s += px[i];
    pvec[i] = s / w;
    s -= px[i - w1];
  }

  return vec;

}

// look - inspect informational attributes
SEXP _look(const SEXP x) {

  SEXP ax, y;
  PROTECT_INDEX pxi;
  PROTECT_WITH_INDEX(y = R_NilValue, &pxi);

  for (ax = ATTRIB(x); ax != R_NilValue; ax = CDR(ax)) {
    if (TAG(ax) != R_NamesSymbol && TAG(ax) != R_RowNamesSymbol &&
        TAG(ax) != R_DimSymbol && TAG(ax) != R_DimNamesSymbol &&
        TAG(ax) != R_ClassSymbol && TAG(ax) != xts_IndexSymbol) {
      REPROTECT(y = Rf_cons(CAR(ax), y), pxi);
      SET_TAG(y, TAG(ax));
    }
  }

  UNPROTECT(1);
  return y;

}

// class object as POSIXct (in-place)
SEXP _psxct(SEXP x) {

  Rf_classgets(x, ichimoku_tclass);
  return x;

}

// ichimoku to data.frame converter
SEXP _tbl(const SEXP x, const SEXP type) {

  const int keepattrs = LOGICAL(type)[0];

  R_xlen_t xlen = 0, xwid = 0;
  const SEXP dims = Rf_getAttrib(x, R_DimSymbol);
  switch (TYPEOF(dims)) {
  case INTSXP:
    xlen = INTEGER(dims)[0];
    xwid = INTEGER(dims)[1];
    break;
  case REALSXP:
    xlen = REAL(dims)[0];
    xwid = REAL(dims)[1];
    break;
  }

  SEXP tbl, index, dn2, names, rownames;

  PROTECT(tbl = Rf_allocVector(VECSXP, xwid + 1));

  index = Rf_shallow_duplicate(Rf_getAttrib(x, xts_IndexSymbol));
  Rf_classgets(index, ichimoku_tclass);
  SET_VECTOR_ELT(tbl, 0, index);

  double *src = REAL(x);
  size_t vecsize = xlen * sizeof(double);
  for (R_xlen_t j = 1; j <= xwid; j++) {
    SEXP vec = Rf_allocVector(REALSXP, xlen);
    SET_VECTOR_ELT(tbl, j, vec);
    memcpy(REAL(vec), src, vecsize);
    src += xlen;
  }

  PROTECT(dn2 = VECTOR_ELT(Rf_getAttrib(x, R_DimNamesSymbol), 1));
  R_xlen_t dlen = Rf_xlength(dn2);
  PROTECT(names = Rf_allocVector(STRSXP, dlen + 1));
  SET_STRING_ELT(names, 0, Rf_mkChar("index"));
  for (R_xlen_t i = 0; i < dlen; i++) {
    SET_STRING_ELT(names, i + 1, STRING_ELT(dn2, i));
  }
  Rf_namesgets(tbl, names);
  UNPROTECT(2);

  Rf_classgets(tbl, Rf_mkString("data.frame"));

  if (xlen <= INT_MAX) {
    rownames = Rf_allocVector(INTSXP, 2);
    INTEGER(rownames)[0] = NA_INTEGER;
    INTEGER(rownames)[1] = -(int) xlen;
  } else {
    rownames = Rf_allocVector(REALSXP, 2);
    REAL(rownames)[0] = NA_REAL;
    REAL(rownames)[1] = -(double) xlen;
  }
  Rf_setAttrib(tbl, R_RowNamesSymbol, rownames);

  if (keepattrs) {
    SEXP ax;
    for (ax = ATTRIB(x); ax != R_NilValue; ax = CDR(ax)) {
      if (TAG(ax) != R_NamesSymbol && TAG(ax) != R_RowNamesSymbol &&
          TAG(ax) != R_DimSymbol && TAG(ax) != R_DimNamesSymbol &&
          TAG(ax) != R_ClassSymbol && TAG(ax) != xts_IndexSymbol)
        Rf_setAttrib(tbl, TAG(ax), CAR(ax));
    }
  }

  UNPROTECT(1);
  return tbl;

}

// internal function used by ichimoku()
SEXP _create(SEXP kumo, SEXP xtsindex, const SEXP periods,
             const SEXP periodicity, const SEXP ticker, const SEXP x) {

  Rf_setAttrib(xtsindex, xts_IndexTzoneSymbol, ichimoku_tzone);
  Rf_setAttrib(xtsindex, xts_IndexTclassSymbol, ichimoku_tclass);
  Rf_setAttrib(kumo, xts_IndexSymbol, xtsindex);

  Rf_classgets(kumo, ichimoku_klass);

  Rf_setAttrib(kumo, ichimoku_PeriodsSymbol, periods);
  Rf_setAttrib(kumo, ichimoku_PeriodicitySymbol, periodicity);
  Rf_setAttrib(kumo, ichimoku_TickerSymbol, ticker);

  if (x != R_NilValue) {
    SEXP ax;
    for (ax = ATTRIB(x); ax != R_NilValue; ax = CDR(ax)) {
      if (TAG(ax) != R_NamesSymbol && TAG(ax) != R_RowNamesSymbol &&
          TAG(ax) != R_DimSymbol && TAG(ax) != R_DimNamesSymbol &&
          TAG(ax) != R_ClassSymbol && TAG(ax) != xts_IndexSymbol &&
          TAG(ax) != ichimoku_PeriodsSymbol &&
          TAG(ax) != ichimoku_PeriodicitySymbol &&
          TAG(ax) != ichimoku_TickerSymbol)
        Rf_setAttrib(kumo, TAG(ax), CAR(ax));
    }
  }

  return kumo;

}

// special ichimoku to data.frame converter for plots
SEXP _df(const SEXP x) {

  R_xlen_t xlen = 0, xwid = 0;
  const SEXP dims = Rf_getAttrib(x, R_DimSymbol);
  switch (TYPEOF(dims)) {
  case INTSXP:
    xlen = INTEGER(dims)[0];
    xwid = INTEGER(dims)[1];
    break;
  case REALSXP:
    xlen = REAL(dims)[0];
    xwid = REAL(dims)[1];
    break;
  }

  if (xwid < 12)
    return R_MissingArg;

  SEXP df, index, idchar, dn2, names, rownames;

  PROTECT(df = Rf_allocVector(VECSXP, xwid + 2));

  index = Rf_shallow_duplicate(Rf_getAttrib(x, xts_IndexSymbol));
  Rf_classgets(index, ichimoku_tclass);
  SET_VECTOR_ELT(df, 0, index);

  double *src = REAL(x);
  size_t vecsize = xlen * sizeof(double);
  for (R_xlen_t j = 1; j <= xwid; j++) {
    SEXP vec = Rf_allocVector(REALSXP, xlen);
    SET_VECTOR_ELT(df, j, vec);
    memcpy(REAL(vec), src, vecsize);
    src += xlen;
  }

  idchar = Rf_coerceVector(VECTOR_ELT(df, 5), STRSXP);
  SET_VECTOR_ELT(df, 5, idchar);

  PROTECT(dn2 = VECTOR_ELT(Rf_getAttrib(x, R_DimNamesSymbol), 1));
  R_xlen_t dlen = Rf_xlength(dn2);
  PROTECT(names = Rf_allocVector(STRSXP, dlen + 2));
  SET_STRING_ELT(names, 0, Rf_mkChar("index"));
  for (R_xlen_t i = 0; i < dlen; i++) {
    SET_STRING_ELT(names, i + 1, STRING_ELT(dn2, i));
  }
  SET_STRING_ELT(names, dlen + 1, Rf_mkChar("idx"));
  Rf_namesgets(df, names);
  UNPROTECT(2);

  Rf_classgets(df, Rf_mkString("data.frame"));

  if (xlen <= INT_MAX) {
    rownames = Rf_allocVector(INTSXP, 2);
    INTEGER(rownames)[0] = NA_INTEGER;
    INTEGER(rownames)[1] = -(int) xlen;
  } else {
    rownames = Rf_allocVector(REALSXP, 2);
    REAL(rownames)[0] = NA_REAL;
    REAL(rownames)[1] = -(double) xlen;
  }
  Rf_setAttrib(df, R_RowNamesSymbol, rownames);

  SET_VECTOR_ELT(df, xwid + 1, Rf_getAttrib(df, R_RowNamesSymbol));

  UNPROTECT(1);
  return df;

}

// ichimoku index method
SEXP _index(SEXP x) {

  SEXP idx = Rf_shallow_duplicate(Rf_getAttrib(x, xts_IndexSymbol));
  Rf_classgets(idx, ichimoku_tclass);
  return idx;

}

// ichimoku coredata method
SEXP _coredata(const SEXP x) {

  SEXP core;
  PROTECT(core = R_shallow_duplicate_attr(x));
  SET_ATTRIB(core, R_NilValue);
  SET_OBJECT(core, 0);
  Rf_dimgets(core, Rf_getAttrib(x, R_DimSymbol));
  Rf_dimnamesgets(core, Rf_getAttrib(x, R_DimNamesSymbol));
  UNPROTECT(1);
  return core;

}

// imports from the package 'xts'
SEXP _naomit(SEXP x) {
  c_fun fun = (c_fun) R_GetCCallable("xts", "na_omit_xts");
  return fun(x);
}

// package level registrations
static void RegisterSymbols(void) {
  xts_IndexSymbol = Rf_install("index");
  xts_IndexTclassSymbol = Rf_install("tclass");
  xts_IndexTzoneSymbol = Rf_install("tzone");
  ichimoku_PeriodsSymbol = Rf_install("periods");
  ichimoku_PeriodicitySymbol = Rf_install("periodicity");
  ichimoku_TickerSymbol = Rf_install("ticker");
}

static void PreserveObjects(void) {
  R_PreserveObject(ichimoku_klass = Rf_allocVector(STRSXP, 3));
  SET_STRING_ELT(ichimoku_klass, 0, Rf_mkChar("ichimoku"));
  SET_STRING_ELT(ichimoku_klass, 1, Rf_mkChar("xts"));
  SET_STRING_ELT(ichimoku_klass, 2, Rf_mkChar("zoo"));
  R_PreserveObject(ichimoku_tclass = Rf_allocVector(STRSXP, 2));
  SET_STRING_ELT(ichimoku_tclass, 0, Rf_mkChar("POSIXct"));
  SET_STRING_ELT(ichimoku_tclass, 1, Rf_mkChar("POSIXt"));
  R_PreserveObject(ichimoku_tzone = Rf_mkString(""));
}

static void ReleaseObjects(void) {
  R_ReleaseObject(ichimoku_tzone);
  R_ReleaseObject(ichimoku_tclass);
  R_ReleaseObject(ichimoku_klass);
}

static const R_CallMethodDef CallEntries[] = {
  {"_coredata", (DL_FUNC) &_coredata, 1},
  {"_create", (DL_FUNC) &_create, 6},
  {"_df", (DL_FUNC) &_df, 1},
  {"_index", (DL_FUNC) &_index, 1},
  {"_look", (DL_FUNC) &_look, 1},
  {"_naomit", (DL_FUNC) &_naomit, 1},
  {"_psxct", (DL_FUNC) &_psxct, 1},
  {"_tbl", (DL_FUNC) &_tbl, 2},
  {"_wmax", (DL_FUNC) &_wmax, 2},
  {"_wmean", (DL_FUNC) &_wmean, 2},
  {"_wmin", (DL_FUNC) &_wmin, 2},
  {NULL, NULL, 0}
};

void attribute_visible R_init_ichimoku(DllInfo* dll) {
  RegisterSymbols();
  PreserveObjects();
  R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
}

void attribute_visible R_unload_ichimoku(DllInfo *info) {
  ReleaseObjects();
}
