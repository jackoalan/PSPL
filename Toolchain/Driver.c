//
//  Driver.c
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//

#include <stdio.h>
#include <string.h>
#include <PSPL/PSPL.h>

struct test_struct {
    uint16_t u_sixteen_field;
    int16_t s_sixteen_field;
    uint32_t u_thirtytwo_field;
    int32_t s_thirtytwo_field;
    float float_field;
    double double_field;
};

typedef DECL_BI_STRUCT(struct test_struct) bi_test_t;

int main(int argc, char** argv) {
    bi_test_t test_inst;
    memset(&test_inst, 0, sizeof(test_inst));
    SET_BI(test_inst, u_sixteen_field, -530);
    SET_BI(test_inst, s_sixteen_field, -530);
    SET_BI(test_inst, u_thirtytwo_field, -500000);
    SET_BI(test_inst, s_thirtytwo_field, -500000);
    SET_BI(test_inst, float_field, 30);
    SET_BI(test_inst, double_field, 60);
    FILE* outf = fopen("output.psplb", "w");
    fwrite(&test_inst, 1, sizeof(test_inst), outf);
    fclose(outf);
    return 0;
}