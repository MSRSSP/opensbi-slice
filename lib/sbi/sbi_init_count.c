#include <sbi/sbi_hart.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_string.h>

unsigned long init_count_offset;

unsigned long sbi_init_count(u32 hartid)
{
	struct sbi_scratch *scratch;
	unsigned long *init_count;

	if (!init_count_offset)
		return 0;

	scratch = sbi_hartid_to_scratch(hartid);
	if (!scratch)
		return 0;

	init_count = sbi_scratch_offset_ptr(scratch, init_count_offset);

	return *init_count;
}
