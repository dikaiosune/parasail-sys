/**
 * @file
 *
 * @author jeff.daily@pnnl.gov
 *
 * Copyright (c) 2015 Battelle Memorial Institute.
 */
#include "config.h"

#include <stdint.h>
#include <stdlib.h>

#include <immintrin.h>

#include "parasail.h"
#include "parasail/memory.h"
#include "parasail/internal_avx.h"

#define NEG_INF (INT16_MIN/(int16_t)(2))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define _mm256_cmplt_epi16_rpl(a,b) _mm256_cmpgt_epi16(b,a)

#if HAVE_AVX2_MM256_INSERT_EPI16
#define _mm256_insert_epi16_rpl _mm256_insert_epi16
#else
static inline __m256i _mm256_insert_epi16_rpl(__m256i a, int16_t i, int imm) {
    __m256i_16_t A;
    A.m = a;
    A.v[imm] = i;
    return A.m;
}
#endif

#if HAVE_AVX2_MM256_EXTRACT_EPI16
#define _mm256_extract_epi16_rpl _mm256_extract_epi16
#else
static inline int16_t _mm256_extract_epi16_rpl(__m256i a, int imm) {
    __m256i_16_t A;
    A.m = a;
    return A.v[imm];
}
#endif

#define _mm256_rlli_si256_rpl(a,imm) _mm256_alignr_epi8(a, _mm256_permute2x128_si256(a, a, _MM_SHUFFLE(0,0,0,1)), 16-imm)

#define _mm256_srli_si256_rpl(a,imm) _mm256_or_si256(_mm256_slli_si256(_mm256_permute2x128_si256(a, a, _MM_SHUFFLE(3,0,0,1)), 16-imm), _mm256_srli_si256(a, imm))

#define _mm256_slli_si256_rpl(a,imm) _mm256_alignr_epi8(a, _mm256_permute2x128_si256(a, a, _MM_SHUFFLE(0,0,3,0)), 16-imm)

static inline int16_t _mm256_hmax_epi16_rpl(__m256i a) {
    a = _mm256_max_epi16(a, _mm256_permute2x128_si256(a, a, _MM_SHUFFLE(0,0,0,0)));
    a = _mm256_max_epi16(a, _mm256_slli_si256(a, 8));
    a = _mm256_max_epi16(a, _mm256_slli_si256(a, 4));
    a = _mm256_max_epi16(a, _mm256_slli_si256(a, 2));
    return _mm256_extract_epi16_rpl(a, 15);
}


#ifdef PARASAIL_TABLE
static inline void arr_store_si256(
        int *array,
        __m256i vH,
        int32_t t,
        int32_t seglen,
        int32_t d,
        int32_t dlen)
{
    array[( 0*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH,  0);
    array[( 1*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH,  1);
    array[( 2*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH,  2);
    array[( 3*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH,  3);
    array[( 4*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH,  4);
    array[( 5*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH,  5);
    array[( 6*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH,  6);
    array[( 7*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH,  7);
    array[( 8*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH,  8);
    array[( 9*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH,  9);
    array[(10*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH, 10);
    array[(11*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH, 11);
    array[(12*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH, 12);
    array[(13*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH, 13);
    array[(14*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH, 14);
    array[(15*seglen+t)*dlen + d] = (int16_t)_mm256_extract_epi16_rpl(vH, 15);
}
#endif

#ifdef PARASAIL_ROWCOL
static inline void arr_store_col(
        int *col,
        __m256i vH,
        int32_t t,
        int32_t seglen)
{
    col[ 0*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH,  0);
    col[ 1*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH,  1);
    col[ 2*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH,  2);
    col[ 3*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH,  3);
    col[ 4*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH,  4);
    col[ 5*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH,  5);
    col[ 6*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH,  6);
    col[ 7*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH,  7);
    col[ 8*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH,  8);
    col[ 9*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH,  9);
    col[10*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH, 10);
    col[11*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH, 11);
    col[12*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH, 12);
    col[13*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH, 13);
    col[14*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH, 14);
    col[15*seglen+t] = (int16_t)_mm256_extract_epi16_rpl(vH, 15);
}
#endif

#ifdef PARASAIL_TABLE
#define FNAME parasail_sg_stats_table_scan_avx2_256_16
#define PNAME parasail_sg_stats_table_scan_profile_avx2_256_16
#else
#ifdef PARASAIL_ROWCOL
#define FNAME parasail_sg_stats_rowcol_scan_avx2_256_16
#define PNAME parasail_sg_stats_rowcol_scan_profile_avx2_256_16
#else
#define FNAME parasail_sg_stats_scan_avx2_256_16
#define PNAME parasail_sg_stats_scan_profile_avx2_256_16
#endif
#endif

parasail_result_t* FNAME(
        const char * const restrict s1, const int s1Len,
        const char * const restrict s2, const int s2Len,
        const int open, const int gap, const parasail_matrix_t *matrix)
{
    parasail_profile_t *profile = parasail_profile_create_stats_avx_256_16(s1, s1Len, matrix);
    parasail_result_t *result = PNAME(profile, s2, s2Len, open, gap);
    parasail_profile_free(profile);
    return result;
}

parasail_result_t* PNAME(
        const parasail_profile_t * const restrict profile,
        const char * const restrict s2, const int s2Len,
        const int open, const int gap)
{
    int32_t i = 0;
    int32_t j = 0;
    int32_t k = 0;
    int32_t end_query = 0;
    int32_t end_ref = 0;
    int32_t segNum = 0;
    const int s1Len = profile->s1Len;
    const parasail_matrix_t *matrix = profile->matrix;
    const int32_t segWidth = 16;
    const int32_t segLen = (s1Len + segWidth - 1) / segWidth;
    const int32_t offset = (s1Len - 1) % segLen;
    const int32_t position = (segWidth - 1) - (s1Len - 1) / segLen;
    __m256i* const restrict pvP  = (__m256i*)profile->profile16.score;
    __m256i* const restrict pvPm = (__m256i*)profile->profile16.matches;
    __m256i* const restrict pvPs = (__m256i*)profile->profile16.similar;
    __m256i* const restrict pvE  = parasail_memalign___m256i(32, segLen);
    __m256i* const restrict pvHt = parasail_memalign___m256i(32, segLen);
    __m256i* const restrict pvFt = parasail_memalign___m256i(32, segLen);
    __m256i* const restrict pvMt = parasail_memalign___m256i(32, segLen);
    __m256i* const restrict pvSt = parasail_memalign___m256i(32, segLen);
    __m256i* const restrict pvLt = parasail_memalign___m256i(32, segLen);
    __m256i* const restrict pvEx = parasail_memalign___m256i(32, segLen);
    __m256i* const restrict pvH  = parasail_memalign___m256i(32, segLen);
    __m256i* const restrict pvM  = parasail_memalign___m256i(32, segLen);
    __m256i* const restrict pvS  = parasail_memalign___m256i(32, segLen);
    __m256i* const restrict pvL  = parasail_memalign___m256i(32, segLen);
    __m256i vGapO = _mm256_set1_epi16(open);
    __m256i vGapE = _mm256_set1_epi16(gap);
    __m256i vZero = _mm256_setzero_si256();
    __m256i vOne = _mm256_set1_epi16(1);
    __m256i vNegInf = _mm256_set1_epi16(NEG_INF);
    int16_t score = NEG_INF;
    int16_t matches = 0;
    int16_t similar = 0;
    int16_t length = 0;
    __m256i vMaxH = vNegInf;
    __m256i vMaxM = vZero;
    __m256i vMaxS = vZero;
    __m256i vMaxL = vZero;
    __m256i vPosMask = _mm256_cmpeq_epi16(_mm256_set1_epi16(position),
            _mm256_set_epi16(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15));
    const int16_t segLenXgap = -segLen*gap;
    __m256i insert_mask = _mm256_cmpeq_epi16(_mm256_setzero_si256(),
            _mm256_set_epi16(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1));
    __m256i vSegLenXgap_reset = _mm256_blendv_epi8(vNegInf,
            _mm256_set1_epi16(segLenXgap),
            insert_mask);
    
#ifdef PARASAIL_TABLE
    parasail_result_t *result = parasail_result_new_table3(segLen*segWidth, s2Len);
#else
#ifdef PARASAIL_ROWCOL
    parasail_result_t *result = parasail_result_new_rowcol3(segLen*segWidth, s2Len);
#else
    parasail_result_t *result = parasail_result_new();
#endif
#endif

    parasail_memset___m256i(pvM, vZero, segLen);
    parasail_memset___m256i(pvS, vZero, segLen);
    parasail_memset___m256i(pvL, vZero, segLen);

    /* initialize H and E */
    {
        int32_t index = 0;
        for (i=0; i<segLen; ++i) {
            __m256i_16_t h;
            __m256i_16_t e;
            for (segNum=0; segNum<segWidth; ++segNum) {
                h.v[segNum] = 0;
                e.v[segNum] = NEG_INF;
            }
            _mm256_store_si256(&pvH[index], h.m);
            _mm256_store_si256(&pvE[index], e.m);
            ++index;
        }
    }

    /* outer loop over database sequence */
    for (j=0; j<s2Len; ++j) {
        __m256i vE;
        __m256i vHt;
        __m256i vFt;
        __m256i vH;
        __m256i *pvW;
        __m256i vW;
        __m256i *pvC;
        __m256i *pvD;
        __m256i vC;
        __m256i vD;
        __m256i vM;
        __m256i vMp;
        __m256i vMt;
        __m256i vS;
        __m256i vSp;
        __m256i vSt;
        __m256i vL;
        __m256i vLp;
        __m256i vLt;
        __m256i vEx;

        /* calculate E */
        for (i=0; i<segLen; ++i) {
            vH = _mm256_load_si256(pvH+i);
            vE = _mm256_load_si256(pvE+i);
            vE = _mm256_max_epi16(
                    _mm256_sub_epi16(vE, vGapE),
                    _mm256_sub_epi16(vH, vGapO));
            _mm256_store_si256(pvE+i, vE);
        }

        /* calculate Ht */
        vH = _mm256_slli_si256_rpl(_mm256_load_si256(pvH+(segLen-1)), 2);
        vMp= _mm256_slli_si256_rpl(_mm256_load_si256(pvM+(segLen-1)), 2);
        vSp= _mm256_slli_si256_rpl(_mm256_load_si256(pvS+(segLen-1)), 2);
        vLp= _mm256_slli_si256_rpl(_mm256_load_si256(pvL+(segLen-1)), 2);
        vLp= _mm256_add_epi16(vLp, vOne);
        pvW = pvP + matrix->mapper[(unsigned char)s2[j]]*segLen;
        pvC = pvPm+ matrix->mapper[(unsigned char)s2[j]]*segLen;
        pvD = pvPs+ matrix->mapper[(unsigned char)s2[j]]*segLen;
        for (i=0; i<segLen; ++i) {
            /* load values we need */
            vE = _mm256_load_si256(pvE+i);
            vW = _mm256_load_si256(pvW+i);
            /* compute */
            vH = _mm256_add_epi16(vH, vW);
            vHt = _mm256_max_epi16(vH, vE);
            /* statistics */
            vC = _mm256_load_si256(pvC+i);
            vD = _mm256_load_si256(pvD+i);
            vMp = _mm256_add_epi16(vMp, vC);
            vSp = _mm256_add_epi16(vSp, vD);
            vEx = _mm256_cmpgt_epi16(vE, vH);
            vM = _mm256_load_si256(pvM+i);
            vS = _mm256_load_si256(pvS+i);
            vL = _mm256_load_si256(pvL+i);
            vL = _mm256_add_epi16(vL, vOne);
            vMt = _mm256_blendv_epi8(vMp, vM, vEx);
            vSt = _mm256_blendv_epi8(vSp, vS, vEx);
            vLt = _mm256_blendv_epi8(vLp, vL, vEx);
            /* store results */
            _mm256_store_si256(pvHt+i, vHt);
            _mm256_store_si256(pvEx+i, vEx);
            _mm256_store_si256(pvMt+i, vMt);
            _mm256_store_si256(pvSt+i, vSt);
            _mm256_store_si256(pvLt+i, vLt);
            /* prep for next iteration */
            vH = _mm256_load_si256(pvH+i);
            vMp = vM;
            vSp = vS;
            vLp = vL;
        }

        /* calculate Ft */
        vHt = _mm256_slli_si256_rpl(_mm256_load_si256(pvHt+(segLen-1)), 2);
        vFt = vNegInf;
        for (i=0; i<segLen; ++i) {
            vFt = _mm256_sub_epi16(vFt, vGapE);
            vFt = _mm256_max_epi16(vFt, vHt);
            vHt = _mm256_load_si256(pvHt+i);
        }
        {
            __m256i vFt_save = vFt;
            __m256i segLenXgap = vSegLenXgap_reset;
            for (i=0; i<segWidth-1; ++i) {
                __m256i vFtt = _mm256_slli_si256_rpl(vFt, 2);
                segLenXgap = _mm256_rlli_si256_rpl(segLenXgap, 2);
                vFtt = _mm256_add_epi16(vFtt, segLenXgap);
                vFt = _mm256_max_epi16(vFt, vFtt);
            }
            vFt = _mm256_blendv_epi8(vFt_save, vFt, insert_mask);
        }
        vHt = _mm256_slli_si256_rpl(_mm256_load_si256(pvHt+(segLen-1)), 2);
        vFt = _mm256_slli_si256_rpl(vFt, 2);
        vFt = _mm256_insert_epi16_rpl(vFt, NEG_INF, 0);
        for (i=0; i<segLen; ++i) {
            vFt = _mm256_sub_epi16(vFt, vGapE);
            vFt = _mm256_max_epi16(vFt, vHt);
            vHt = _mm256_load_si256(pvHt+i);
            _mm256_store_si256(pvFt+i, vFt);
        }

        /* calculate H,M,L */
        vMp = vZero;
        vSp = vZero;
        vLp = vOne;
        vC = _mm256_cmpeq_epi16(vZero, vZero); /* check if prefix sum is needed */
        vC = _mm256_srli_si256_rpl(vC, 2); /* zero out last value */
        for (i=0; i<segLen; ++i) {
            /* load values we need */
            vHt = _mm256_load_si256(pvHt+i);
            vFt = _mm256_load_si256(pvFt+i);
            /* compute */
            vFt = _mm256_sub_epi16(vFt, vGapO);
            vH = _mm256_max_epi16(vHt, vFt);
            /* statistics */
            vEx = _mm256_load_si256(pvEx+i);
            vMt = _mm256_load_si256(pvMt+i);
            vSt = _mm256_load_si256(pvSt+i);
            vLt = _mm256_load_si256(pvLt+i);
            vEx = _mm256_or_si256(
                    _mm256_and_si256(vEx, _mm256_cmpeq_epi16(vHt, vFt)),
                    _mm256_cmplt_epi16_rpl(vHt, vFt));
            vM = _mm256_blendv_epi8(vMt, vMp, vEx);
            vS = _mm256_blendv_epi8(vSt, vSp, vEx);
            vL = _mm256_blendv_epi8(vLt, vLp, vEx);
            vMp = vM;
            vSp = vS;
            vLp = _mm256_add_epi16(vL, vOne);
            vC = _mm256_and_si256(vC, vEx);
            /* store results */
            _mm256_store_si256(pvH+i, vH);
            _mm256_store_si256(pvEx+i, vEx);
            
#ifdef PARASAIL_TABLE
            arr_store_si256(result->score_table, vH, i, segLen, j, s2Len);
#endif
        }
        {
            vLp = _mm256_sub_epi16(vLp, vOne);
            {
                __m256i_16_t uMp, uSp, uLp, uC;
                uC.m = vC;
                uMp.m = vMp;
                uMp.v[ 1] = uC.v[ 1] ? uMp.v[ 0] : uMp.v[ 1];
                uMp.v[ 2] = uC.v[ 2] ? uMp.v[ 1] : uMp.v[ 2];
                uMp.v[ 3] = uC.v[ 3] ? uMp.v[ 2] : uMp.v[ 3];
                uMp.v[ 4] = uC.v[ 4] ? uMp.v[ 3] : uMp.v[ 4];
                uMp.v[ 5] = uC.v[ 5] ? uMp.v[ 4] : uMp.v[ 5];
                uMp.v[ 6] = uC.v[ 6] ? uMp.v[ 5] : uMp.v[ 6];
                uMp.v[ 7] = uC.v[ 7] ? uMp.v[ 6] : uMp.v[ 7];
                uMp.v[ 8] = uC.v[ 8] ? uMp.v[ 7] : uMp.v[ 8];
                uMp.v[ 9] = uC.v[ 9] ? uMp.v[ 8] : uMp.v[ 9];
                uMp.v[10] = uC.v[10] ? uMp.v[ 9] : uMp.v[10];
                uMp.v[11] = uC.v[11] ? uMp.v[10] : uMp.v[11];
                uMp.v[12] = uC.v[12] ? uMp.v[11] : uMp.v[12];
                uMp.v[13] = uC.v[13] ? uMp.v[12] : uMp.v[13];
                uMp.v[14] = uC.v[14] ? uMp.v[13] : uMp.v[14];
                uMp.v[15] = uC.v[15] ? uMp.v[14] : uMp.v[15];
                vMp = uMp.m;
                uSp.m = vSp;
                uSp.v[ 1] = uC.v[ 1] ? uSp.v[ 0] : uSp.v[ 1];
                uSp.v[ 2] = uC.v[ 2] ? uSp.v[ 1] : uSp.v[ 2];
                uSp.v[ 3] = uC.v[ 3] ? uSp.v[ 2] : uSp.v[ 3];
                uSp.v[ 4] = uC.v[ 4] ? uSp.v[ 3] : uSp.v[ 4];
                uSp.v[ 5] = uC.v[ 5] ? uSp.v[ 4] : uSp.v[ 5];
                uSp.v[ 6] = uC.v[ 6] ? uSp.v[ 5] : uSp.v[ 6];
                uSp.v[ 7] = uC.v[ 7] ? uSp.v[ 6] : uSp.v[ 7];
                uSp.v[ 8] = uC.v[ 8] ? uSp.v[ 7] : uSp.v[ 8];
                uSp.v[ 9] = uC.v[ 9] ? uSp.v[ 8] : uSp.v[ 9];
                uSp.v[10] = uC.v[10] ? uSp.v[ 9] : uSp.v[10];
                uSp.v[11] = uC.v[11] ? uSp.v[10] : uSp.v[11];
                uSp.v[12] = uC.v[12] ? uSp.v[11] : uSp.v[12];
                uSp.v[13] = uC.v[13] ? uSp.v[12] : uSp.v[13];
                uSp.v[14] = uC.v[14] ? uSp.v[13] : uSp.v[14];
                uSp.v[15] = uC.v[15] ? uSp.v[14] : uSp.v[15];
                vSp = uSp.m;
                uLp.m = vLp;
                uLp.v[ 1] = uC.v[ 1] ? uLp.v[ 1] + uLp.v[ 0] : uLp.v[ 1];
                uLp.v[ 2] = uC.v[ 2] ? uLp.v[ 2] + uLp.v[ 1] : uLp.v[ 2];
                uLp.v[ 3] = uC.v[ 3] ? uLp.v[ 3] + uLp.v[ 2] : uLp.v[ 3];
                uLp.v[ 4] = uC.v[ 4] ? uLp.v[ 4] + uLp.v[ 3] : uLp.v[ 4];
                uLp.v[ 5] = uC.v[ 5] ? uLp.v[ 5] + uLp.v[ 4] : uLp.v[ 5];
                uLp.v[ 6] = uC.v[ 6] ? uLp.v[ 6] + uLp.v[ 5] : uLp.v[ 6];
                uLp.v[ 7] = uC.v[ 7] ? uLp.v[ 7] + uLp.v[ 6] : uLp.v[ 7];
                uLp.v[ 8] = uC.v[ 8] ? uLp.v[ 8] + uLp.v[ 7] : uLp.v[ 8];
                uLp.v[ 9] = uC.v[ 9] ? uLp.v[ 9] + uLp.v[ 8] : uLp.v[ 9];
                uLp.v[10] = uC.v[10] ? uLp.v[10] + uLp.v[ 9] : uLp.v[10];
                uLp.v[11] = uC.v[11] ? uLp.v[11] + uLp.v[10] : uLp.v[11];
                uLp.v[12] = uC.v[12] ? uLp.v[12] + uLp.v[11] : uLp.v[12];
                uLp.v[13] = uC.v[13] ? uLp.v[13] + uLp.v[12] : uLp.v[13];
                uLp.v[14] = uC.v[14] ? uLp.v[14] + uLp.v[13] : uLp.v[14];
                uLp.v[15] = uC.v[15] ? uLp.v[15] + uLp.v[14] : uLp.v[15];
                vLp = uLp.m;
            }
        }
        /* final pass for M,L */
        vMp = _mm256_slli_si256_rpl(vMp, 2);
        vSp = _mm256_slli_si256_rpl(vSp, 2);
        vLp = _mm256_slli_si256_rpl(vLp, 2);
        vLp = _mm256_add_epi16(vLp, vOne);
        for (i=0; i<segLen; ++i) {
            /* statistics */
            vEx = _mm256_load_si256(pvEx+i);
            vMt = _mm256_load_si256(pvMt+i);
            vSt = _mm256_load_si256(pvSt+i);
            vLt = _mm256_load_si256(pvLt+i);
            vM = _mm256_blendv_epi8(vMt, vMp, vEx);
            vS = _mm256_blendv_epi8(vSt, vSp, vEx);
            vL = _mm256_blendv_epi8(vLt, vLp, vEx);
            vMp = vM;
            vSp = vS;
            vLp = _mm256_add_epi16(vL, vOne);
            /* store results */
            _mm256_store_si256(pvM+i, vM);
            _mm256_store_si256(pvS+i, vS);
            _mm256_store_si256(pvL+i, vL);
            
#ifdef PARASAIL_TABLE
            arr_store_si256(result->matches_table, vM, i, segLen, j, s2Len);
            arr_store_si256(result->similar_table, vS, i, segLen, j, s2Len);
            arr_store_si256(result->length_table, vL, i, segLen, j, s2Len);
#endif
        }

        /* extract vector containing last value from column */
        {
            __m256i cond_max;
            vH = _mm256_load_si256(pvH + offset);
            vM = _mm256_load_si256(pvM + offset);
            vS = _mm256_load_si256(pvS + offset);
            vL = _mm256_load_si256(pvL + offset);
            cond_max = _mm256_cmpgt_epi16(vH, vMaxH);
            vMaxH = _mm256_blendv_epi8(vMaxH, vH, cond_max);
            vMaxM = _mm256_blendv_epi8(vMaxM, vM, cond_max);
            vMaxS = _mm256_blendv_epi8(vMaxS, vS, cond_max);
            vMaxL = _mm256_blendv_epi8(vMaxL, vL, cond_max);
            if (_mm256_movemask_epi8(_mm256_and_si256(vPosMask, cond_max))) {
                end_ref = j;
                end_query = s1Len - 1;
            }
#ifdef PARASAIL_ROWCOL
            for (k=0; k<position; ++k) {
                vH = _mm256_slli_si256_rpl(vH, 2);
                vM = _mm256_slli_si256_rpl(vM, 2);
                vS = _mm256_slli_si256_rpl(vS, 2);
                vL = _mm256_slli_si256_rpl(vL, 2);
            }
            result->score_row[j] = (int16_t) _mm256_extract_epi16_rpl (vH, 15);
            result->matches_row[j] = (int16_t) _mm256_extract_epi16_rpl (vM, 15);
            result->similar_row[j] = (int16_t) _mm256_extract_epi16_rpl (vS, 15);
            result->length_row[j] = (int16_t) _mm256_extract_epi16_rpl (vL, 15);
#endif
        }
    }

    /* max last value from all columns */
    {
        for (k=0; k<position; ++k) {
            vMaxH = _mm256_slli_si256_rpl(vMaxH, 2);
            vMaxM = _mm256_slli_si256_rpl(vMaxM, 2);
            vMaxS = _mm256_slli_si256_rpl(vMaxS, 2);
            vMaxL = _mm256_slli_si256_rpl(vMaxL, 2);
        }
        score = (int16_t) _mm256_extract_epi16_rpl(vMaxH, 15);
        matches = (int16_t) _mm256_extract_epi16_rpl(vMaxM, 15);
        similar = (int16_t) _mm256_extract_epi16_rpl(vMaxS, 15);
        length = (int16_t) _mm256_extract_epi16_rpl(vMaxL, 15);
    }

    /* max of last column */
    {
        int16_t score_last;
        vMaxH = vNegInf;

        for (i=0; i<segLen; ++i) {
            /* load the last stored values */
            __m256i vH = _mm256_load_si256(pvH + i);
#ifdef PARASAIL_ROWCOL
            __m256i vM = _mm256_load_si256(pvM + i);
            __m256i vS = _mm256_load_si256(pvS + i);
            __m256i vL = _mm256_load_si256(pvL + i);
            arr_store_col(result->score_col, vH, i, segLen);
            arr_store_col(result->matches_col, vM, i, segLen);
            arr_store_col(result->similar_col, vS, i, segLen);
            arr_store_col(result->length_col, vL, i, segLen);
#endif
            vMaxH = _mm256_max_epi16(vH, vMaxH);
        }

        /* max in vec */
        score_last = _mm256_hmax_epi16_rpl(vMaxH);
        if (score_last > score) {
            score = score_last;
            end_ref = s2Len - 1;
            end_query = s1Len;
            /* Trace the alignment ending position on read. */
            {
                int16_t *t = (int16_t*)pvH;
                int16_t *m = (int16_t*)pvM;
                int16_t *s = (int16_t*)pvS;
                int16_t *l = (int16_t*)pvL;
                int32_t column_len = segLen * segWidth;
                for (i = 0; i<column_len; ++i, ++t, ++m, ++s, ++l) {
                    if (*t == score) {
                        int32_t temp = i / segWidth + i % segWidth * segLen;
                        if (temp < end_query) {
                            end_query = temp;
                            matches = *m;
                            similar = *s;
                            length = *l;
                        }
                    }
                }
            }
        }
    }

    

    result->score = score;
    result->matches = matches;
    result->similar = similar;
    result->length = length;
    result->end_query = end_query;
    result->end_ref = end_ref;

    parasail_free(pvL);
    parasail_free(pvS);
    parasail_free(pvM);
    parasail_free(pvH);
    parasail_free(pvEx);
    parasail_free(pvLt);
    parasail_free(pvSt);
    parasail_free(pvMt);
    parasail_free(pvFt);
    parasail_free(pvHt);
    parasail_free(pvE);

    return result;
}


