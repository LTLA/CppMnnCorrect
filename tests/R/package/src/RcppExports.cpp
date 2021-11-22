// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

#ifdef RCPP_USE_GLOBAL_ROSTREAM
Rcpp::Rostream<true>&  Rcpp::Rcout = Rcpp::Rcpp_cout_get();
Rcpp::Rostream<false>& Rcpp::Rcerr = Rcpp::Rcpp_cerr_get();
#endif

// mnn_correct
Rcpp::List mnn_correct(Rcpp::NumericMatrix x, Rcpp::IntegerVector batch, int k, double nmads, int iterations, double trim);
RcppExport SEXP _mnncorrect_ref_mnn_correct(SEXP xSEXP, SEXP batchSEXP, SEXP kSEXP, SEXP nmadsSEXP, SEXP iterationsSEXP, SEXP trimSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::traits::input_parameter< Rcpp::NumericMatrix >::type x(xSEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type batch(batchSEXP);
    Rcpp::traits::input_parameter< int >::type k(kSEXP);
    Rcpp::traits::input_parameter< double >::type nmads(nmadsSEXP);
    Rcpp::traits::input_parameter< int >::type iterations(iterationsSEXP);
    Rcpp::traits::input_parameter< double >::type trim(trimSEXP);
    rcpp_result_gen = Rcpp::wrap(mnn_correct(x, batch, k, nmads, iterations, trim));
    return rcpp_result_gen;
END_RCPP
}
// robust_average
Rcpp::NumericVector robust_average(Rcpp::NumericMatrix x, int iterations, double trim);
RcppExport SEXP _mnncorrect_ref_robust_average(SEXP xSEXP, SEXP iterationsSEXP, SEXP trimSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::traits::input_parameter< Rcpp::NumericMatrix >::type x(xSEXP);
    Rcpp::traits::input_parameter< int >::type iterations(iterationsSEXP);
    Rcpp::traits::input_parameter< double >::type trim(trimSEXP);
    rcpp_result_gen = Rcpp::wrap(robust_average(x, iterations, trim));
    return rcpp_result_gen;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_mnncorrect_ref_mnn_correct", (DL_FUNC) &_mnncorrect_ref_mnn_correct, 6},
    {"_mnncorrect_ref_robust_average", (DL_FUNC) &_mnncorrect_ref_robust_average, 3},
    {NULL, NULL, 0}
};

RcppExport void R_init_mnncorrect_ref(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
