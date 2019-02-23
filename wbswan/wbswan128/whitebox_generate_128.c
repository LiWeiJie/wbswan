#include <stdio.h>
#include <math/affine_transform.h>
#include <math/matrix_utils.h>
#include <wbswan/wbswan.h>
#include <time.h>
#define DELTA 0x9e3779b97f4a7c15
static unsigned char S[16] = {0x01, 0x02, 0x0C, 0x05, 0x07, 0x08, 0x0A, 0x0F, 0x04, 0x0D, 0x0B, 0x0E, 0x09, 0x06, 0x00, 0x03};

static MatGf2 make_right_rotate_shift(int dim, int r1, int r2, int r3)
{
    MatGf2 ind = GenMatGf2(dim, dim);
    int i;
    for (i = 0; i < 8; i++)
    {
        MatGf2Set(ind, i, ((i + 8 - r3) % 8 + 0), 1);
    }
    for (i = 8; i < 16; i++)
    {
        MatGf2Set(ind, i, (i + 8 - r2) % 8 + 8, 1);
    }
    for (i = 16; i < 24; i++)
    {
        MatGf2Set(ind, i, (i + 8 - r1) % 8 + 16, 1);
    }
    for (i = 24; i < 32; i++)
    {
        MatGf2Set(ind, i, (i + 8 - 0) % 8 + 24, 1);
    }

    return ind;
}

static MatGf2 make_singlerow_right_rotate_shift(int dim, int r)
{
    MatGf2 ind = GenMatGf2(dim, dim);
    int i;
    for (i = 0; i < dim; i++)
    {
        MatGf2Set(ind, i, (i + r) % dim, 1);
    }
    return ind;
}

static MatGf2 make_special_rotate_back(int dim)
{
    uint8_t rot[8] = {24, 28, 16, 20, 8, 12, 0, 4};
    MatGf2 ind = GenMatGf2(dim, dim);
    int i;
    int j;
    int row = 0;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 8; j++)
        {
            MatGf2Set(ind, row++, rot[j] + i, 1);

        }
    }
    return ind;
}

static MatGf2 make_special_rotate(int dim)
{
    uint8_t rot[8] = {0, 8, 16, 24, 1, 9, 17, 25};
    MatGf2 ind = GenMatGf2(dim, dim);
    int i;
    int j;
    int row;
    for (i = 1; i <= 4; i++)
    {
        row = 32 - 8 * i;

        for (j = 0; j < 8; j++)
        {
            MatGf2Set(ind, row++, rot[j] + 2 * (i - 1), 1);
           
        }
    }
    return ind;
}

static MatGf2 make_right_rotate_shift_64(int dim, int r1, int r2, int r3)
{
    MatGf2 ind = GenMatGf2(dim, dim);
    int i;
    for (i = 0; i < 16; i++)
    {
        MatGf2Set(ind, i, ((i + 16 - r3) % 16 + 0), 1);
    }
    for (i = 16; i < 32; i++)
    {
        MatGf2Set(ind, i, (i + 16 - r2) % 16 + 16, 1);
    }
    for (i = 32; i < 48; i++)
    {
        MatGf2Set(ind, i, (i + 16 - r1) % 16 + 32, 1);
    }
    for (i = 48; i < 54; i++)
    {
        MatGf2Set(ind, i, (i + 16 - 0) % 16 + 48, 1);
    }

    return ind;
}
static MatGf2 make_special_rotate_64(int dim)
{
    uint8_t rot[16] = {0, 16, 32, 48, 1, 17, 33, 49, 2, 18, 34, 50, 3, 19, 35, 51};
    MatGf2 ind = GenMatGf2(dim, dim);
    int i;
    int j;
    int row;
    for (i = 1; i <= 4; i++)
    {
        row = 64 - 16 * i;

        for (j = 0; j < 16; j++)
        {
            MatGf2Set(ind, row++, rot[j] + 4 * (i - 1), 1);
        }
    }
    return ind;
}

static MatGf2 make_special_rotate_back_64(int dim)
{
    uint8_t rot[16] = {48, 52, 56, 60, 32, 36, 40, 44, 16, 20, 24, 28, 0, 4, 8, 12};
    MatGf2 ind = GenMatGf2(dim, dim);
    int i;
    int j;
    int row = 0;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 16; j++)
        {
            MatGf2Set(ind, row++, rot[j] + i, 1);
        }
    }
    return ind;
}

void RotateKeyByte(uint8_t *key, uint16_t keylength)
{
    uint8_t i;
    uint8_t temp[7];
    uint8_t N = keylength / 8 - 1;
    for (i = 0; i < 7; i++)
    {
        temp[6 - i] = key[N - i];
    }

    //Right rotate every byte of the key;
    for (i = N; i >= 7; i--)
    {
        key[i] = key[i - 7];
    }

    //Cyclic the first byte of the key to the MSB;
    for (i = 0; i < 7; i++)
    {
        key[i] = temp[i];
    }
}

void InvRotateKeyByte(uint8_t *key, uint16_t keylength)
{
    uint8_t i;
    uint8_t temp[7];
    uint8_t N = keylength / 8 - 1;
    for (i = 0; i < 7; i++)
    {
        temp[i] = key[i];
    }

    //Right rotate every byte of the key;
    for (i = 0; i <= N - 7; i++)
    {
        key[i] = key[i + 7];
    }

    //Cyclic the first byte of the key to the MSB;
    for (i = 0; i < 7; i++)
    {
        key[(N + 1) - 7 + i] = temp[i];
    }
}

void AddRoundConstant(uint16_t *subkey, uint64_t sum)
{

    uint64_t *a = (uint64_t *)subkey;
    uint16_t *b = (uint16_t *)a;
    (*a) = (*a) + sum;

    subkey[0] = b[0];
    subkey[1] = b[1];
    subkey[2] = b[2];
    subkey[3] = b[3];
}
int Key_Schedule(unsigned char *Seedkey, int KeyLen, unsigned char Direction, unsigned char *Subkey,int rounds)
{
    int i;
    uint16_t *key;
    uint16_t subkey[4];
    uint64_t round_constant = 0;
    uint16_t(*ekey)[4] = (uint16_t(*)[4])malloc(sizeof(uint16_t) * (rounds) * 4);
    key = (uint16_t *)malloc(sizeof(uint16_t) * (KeyLen / 8));
    memcpy(key, Seedkey, (KeyLen / 8) * sizeof(uint16_t));
    MatGf2 mat = make_special_rotate_64(64);
    MatGf2 mat_back = make_special_rotate_back_64(64);
    for (i = 0; i < rounds; i++)
    {
        RotateKeyByte(key, KeyLen);
        subkey[0] = key[0];
        subkey[1] = key[1];
        subkey[2] = key[2];
        subkey[3] = key[3];
        round_constant = round_constant + DELTA;
        AddRoundConstant(subkey, round_constant);

        *((uint64_t *)subkey) = ApplyMatToU64(mat, *((uint64_t *)subkey));

        ekey[i][0] = subkey[0];
        ekey[i][1] = subkey[1];
        ekey[i][2] = subkey[2];
        ekey[i][3] = subkey[3];

        *((uint64_t *)subkey) = ApplyMatToU64(mat_back, *((uint64_t *)subkey));

        key[0] = subkey[0];
        key[1] = subkey[1];
        key[2] = subkey[2];
        key[3] = subkey[3];
    }
    memcpy(Subkey, ekey, (rounds) * 4 * sizeof(uint16_t));
    free(key);
    free(ekey);
    MatGf2Free(mat);
    MatGf2Free(mat_back);
    mat = NULL;
    mat_back = NULL;
    return 0;
}

static MatGf2 make_swithlane_mat(int dim)
{
    MatGf2 ind = GenMatGf2(dim, dim);
    int i;
    for (i = 0; i < 8; i++)
    {
        MatGf2Set(ind, i, i + 8, 1);
        MatGf2Set(ind, i, i + 16, 1);
        MatGf2Set(ind, i, i + 24, 1);
    }
    for (i = 8; i < 16; i++)
    {
        MatGf2Set(ind, i, i - 8, 1);
        MatGf2Set(ind, i, i + 16, 1);
        MatGf2Set(ind, i, i + 8, 1);
    }
    for (i = 16; i < 24; i++)
    {
        MatGf2Set(ind, i, i + 8, 1);
        MatGf2Set(ind, i, i - 16, 1);
        MatGf2Set(ind, i, i - 8, 1);
    }
    for (i = 24; i < 32; i++)
    {
        MatGf2Set(ind, i, i - 8, 1);
        MatGf2Set(ind, i, i - 16, 1);
        MatGf2Set(ind, i, i - 24, 1);
    }

    return ind;
}

int _swan_whitebox_helper_init(swan_whitebox_helper *swh, uint8_t *key)
{

    int block_size = swh->block_size;
    int semi_block = block_size / 2;
    swh->piece_count = semi_block / SWAN_PIECE_BIT;
    swh->key = key;


    return 0;
}

int swan_whitebox_64_helper_init(const uint8_t *key, swan_whitebox_helper *swh,int enc)
// designed for swan_64_128
{

    // uint8_t rk[MAX_RK_SIZE];
    int ret;
    swh->cfg = CFG;
    swh->encrypt = enc;
    swh->rounds = swan_cfg_rounds[swh->cfg];
    swh->block_size = swan_cfg_blocksizes[swh->cfg];
    return _swan_whitebox_helper_init(swh, key);
}

int _swan_whitebox_content_init(swan_whitebox_helper *swh, swan_whitebox_content *swc)
{
    // TODO:
    swc->cfg = swh->cfg;
    swc->rounds = swh->rounds;
    swc->block_size = swh->block_size;
    swc->piece_count = swh->piece_count;

    // swc->lut = (piece_t *)malloc(swc->rounds * 2 * swc->piece_count * sizeof(piece_t));

    swc->lut = (swan_wb_semi(*)[4][2][256])malloc(sizeof(swan_wb_semi) * swc->rounds * 256 * 4 * 2);
    swc->P = (CombinedAffine *)malloc((swh->rounds + 2)  * sizeof(CombinedAffine));
    swc->B = (CombinedAffine *)malloc(swh->rounds * sizeof(CombinedAffine));
    swc->C = (CombinedAffine *)malloc(swh->rounds * sizeof(CombinedAffine));
    int i;
    CombinedAffine *B_ptr = swc->B;
    CombinedAffine *C_ptr = swc->C;
    CombinedAffine *P_ptr = swc->P;

    for (i = 0; i < (swh->rounds); i++)
    {

        combined_affine_init(B_ptr++, SWAN_PIECE_BIT, swh->piece_count);
        combined_affine_init(C_ptr++, SWAN_PIECE_BIT, swh->piece_count);
       
    }
    for(i = 0;i < (swh->rounds + 2); i++){
        combined_affine_init(P_ptr++, SWAN_PIECE_BIT, swh->piece_count);
    }
 

    return 0;
}

int _swan_whitebox_content_assemble(swan_whitebox_helper *swh, swan_whitebox_content *swc)
{
    int piece_count = swh->piece_count;
    int i, j, k, r;
    int piece_range = 1 << SWAN_PIECE_BIT;
    
    /* theta */
    MatGf2 special = make_special_rotate(piece_count * SWAN_PIECE_BIT);
    MatGf2 rotate = make_right_rotate_shift(piece_count * SWAN_PIECE_BIT, ROL_A[swh->cfg], ROL_B[swh->cfg], ROL_C[swh->cfg]);
    MatGf2Mul(special, rotate, &rotate);

    CombinedAffine *P_ptr = swc->P;
    CombinedAffine *B_ptr = swc->B;
    CombinedAffine *C_ptr = swc->C;
    MatGf2 temp;
    for (i = 0; i < swh->rounds ; i++)
    {
        //B * rotate * P'* X + B * rotate * p + b

        temp = GenMatGf2Mul(B_ptr->combined_affine->linear_map, rotate);
        B_ptr->combined_affine->linear_map = GenMatGf2Mul(temp, P_ptr->combined_affine_inv->linear_map);
        B_ptr->combined_affine->vector_translation = GenMatGf2Add(GenMatGf2Mul(temp, P_ptr->combined_affine_inv->vector_translation), B_ptr->combined_affine->vector_translation);
        P_ptr++;
        B_ptr++;
    }

   
    B_ptr = swc->B;
    P_ptr = swc->P+1;
    MatGf2 rotate_back = make_special_rotate_back_64(piece_count * SWAN_PIECE_BIT);
    swan_wb_unit key_schedule[64][4] ;
    Key_Schedule(swh->key, swan_cfg_keysizes[swh->cfg], 1, key_schedule,swh->rounds);
    srand(time(NULL));
    /* w */

    if(swh->encrypt == 1){
        for (r = 0; r < swh->rounds; r++)
        {

            uint16_t randomNum[4][4][2];

            int j = 0;
            int t = 0;
            // random
            int num = 0;
            int y;
            for (j = 0; j < 4; j++)
            {
                for (t = 0; t < 4; t++)
                {
                    for (y = 0; y < 2; y++)
                    {
                        randomNum[j][t][y] = num;
                    }
                }
            }

            // P_ptr->combined_affine->linear_map = GenMatGf2Mul(P_ptr->combined_affine->linear_map,);

            for (k = 0; k < piece_count; k++)
            {
                uint16_t switchxor = 0;
                for (t = 0; t < 4; t++)
                {

                    if (t != k)
                    {

                        switchxor = switchxor ^ randomNum[t][t][0] ^ randomNum[t][t][1];
                    }
                }
                C_ptr->sub_affine[k].vector_translation = GenMatAddU16(C_ptr->sub_affine[k].vector_translation, ApplyMatToU16(C_ptr->sub_affine[k].linear_map, switchxor));

                for (i = 0; i < 256; i++)
                {
                    int n;
                    uint16_t initVec = ApplyAffineToU16((B_ptr)->sub_affine_inv[k], i) ^ key_schedule[r][k];
                    for (n = 0; n < 2; n++)
                    {

                        uint8_t t8 = n ? ((initVec >> 8) & 0x00ff) : (initVec & 0x00ff);
                        uint8_t temp;
                        uint16_t yc[4] = {0};
                        temp = (t8 >> 4) & 0x0f;
                        t8 = (S[(t8 >> 4) & 0x0f]) << 4 | (S[t8 & 0x0f]);
                        int t = 0;

                        yc[k] = t8;

                        *((uint64_t *)yc) = ApplyMatToU64(rotate_back, *((uint64_t *)yc));

                        if (k != 3)
                        {
                            for (j = 0; j < 4; j++)
                            {
                                yc[j] = yc[j] ^ randomNum[k][j][n];
                            }
                        }
                        else
                        {
                            yc[0] = yc[0] ^ randomNum[1][0][n] ^ randomNum[2][0][n];
                            yc[1] = yc[1] ^ randomNum[0][1][n] ^ randomNum[2][1][n];
                            yc[2] = yc[2] ^ randomNum[0][2][n] ^ randomNum[1][2][n];
                            yc[3] = yc[3] ^ randomNum[0][3][n] ^ randomNum[1][3][n] ^ randomNum[2][3][n] ^ randomNum[3][3][n];
                        }

                        // rotate = make_right_rotate_shift(piece_count * SWAN_PIECE_BIT, ROL_A[swh->cfg], ROL_B[swh->cfg], ROL_C[swh->cfg]);

                        swc->lut[r][k][n][i] = *((uint64_t *)yc);
                    }
                }
            }
            B_ptr++;
            C_ptr++;
            P_ptr++;
        }
    }
    else {
        for (r = swh->rounds - 1; r >= 0; r--)
        {

            uint16_t randomNum[4][4][2];

            int j = 0;
            int t = 0;
            // random
            int num = 0;
            int y;
            for (j = 0; j < 4; j++)
            {
                for (t = 0; t < 4; t++)
                {
                    for (y = 0; y < 2; y++)
                    {
                        randomNum[j][t][y] = num;
                    }
                }
            }

            // P_ptr->combined_affine->linear_map = GenMatGf2Mul(P_ptr->combined_affine->linear_map,);

            for (k = 0; k < piece_count; k++)
            {
                uint16_t switchxor = 0;
                for (t = 0; t < 4; t++)
                {

                    if (t != k)
                    {

                        switchxor = switchxor ^ randomNum[t][t][0] ^ randomNum[t][t][1];
                    }
                }
                C_ptr->sub_affine[k].vector_translation = GenMatAddU16(C_ptr->sub_affine[k].vector_translation, ApplyMatToU16(C_ptr->sub_affine[k].linear_map, switchxor));

                for (i = 0; i < 256; i++)
                {
                    int n;
                    uint16_t initVec = ApplyAffineToU16((B_ptr)->sub_affine_inv[k], i) ^ key_schedule[r][k];
                    for (n = 0; n < 2; n++)
                    {

                        uint8_t t8 = n ? ((initVec >> 8) & 0x00ff) : (initVec & 0x00ff);
                        uint8_t temp;
                        uint16_t yc[4] = {0};
                        temp = (t8 >> 4) & 0x0f;
                        t8 = (S[(t8 >> 4) & 0x0f]) << 4 | (S[t8 & 0x0f]);
                        int t = 0;

                        yc[k] = t8;

                        *((uint64_t *)yc) = ApplyMatToU64(rotate_back, *((uint64_t *)yc));

                        if (k != 3)
                        {
                            for (j = 0; j < 4; j++)
                            {
                                yc[j] = yc[j] ^ randomNum[k][j][n];
                            }
                        }
                        else
                        {
                            yc[0] = yc[0] ^ randomNum[1][0][n] ^ randomNum[2][0][n];
                            yc[1] = yc[1] ^ randomNum[0][1][n] ^ randomNum[2][1][n];
                            yc[2] = yc[2] ^ randomNum[0][2][n] ^ randomNum[1][2][n];
                            yc[3] = yc[3] ^ randomNum[0][3][n] ^ randomNum[1][3][n] ^ randomNum[2][3][n] ^ randomNum[3][3][n];
                        }

                        // rotate = make_right_rotate_shift(piece_count * SWAN_PIECE_BIT, ROL_A[swh->cfg], ROL_B[swh->cfg], ROL_C[swh->cfg]);

                        swc->lut[r][k][n][i] = *((uint64_t *)yc);
                    }
                }
            }
            B_ptr++;
            C_ptr++;
            P_ptr++;
        }
    }

   
  


    MatGf2Free(special);
    special = NULL;
    MatGf2Free(rotate);
    rotate = NULL;
    return 0;
}

int swan_whitebox_64_init(const uint8_t *key, int enc, swan_whitebox_content *swc)
{
    swan_whitebox_helper *swh = (swan_whitebox_helper *)malloc(sizeof(swan_whitebox_helper));
    swan_whitebox_64_helper_init(key, swh,enc);
    swan_whitebox_64_content_init(swh, swc);
    swan_whitebox_64_content_assemble(swh, swc);
   
    return 0;
}
int swan_whitebox_64_content_init(swan_whitebox_helper *swh, swan_whitebox_content *swc)
{
    return _swan_whitebox_content_init(swh, swc);
}
int swan_whitebox_64_content_assemble(swan_whitebox_helper *swh, swan_whitebox_content *swc)
{
    return _swan_whitebox_content_assemble(swh, swc);
}
