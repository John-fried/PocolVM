#include <stdio.h>
#include <stdint.h>

#define DESC_PACK(op1, op2) (((op2) << 4) | (op1))  /* pack descriptor operand 1 & 2*/
#define DESC_GET_OP1(desc)  ((desc) & 0x0F)         /* get operand 1; 0x0F: 0000 1111 */
#define DESC_GET_OP2(desc)  ((desc) >> 4)           /* get operand 2*/

int main(void)
{
	uint8_t desc = DESC_PACK(0x01, 0x02);
	printf("desc: %d\n", desc);
	uint8_t op1 = DESC_GET_OP1(desc);
	uint8_t op2 = DESC_GET_OP2(desc);
	printf("op1: %d\nop2: %d", op1, op2);
}
