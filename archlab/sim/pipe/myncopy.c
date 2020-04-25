typedef int word_t;
word_t ncopy2(word_t *src, word_t *dst, word_t len)
{
    word_t count = 0;
    word_t val;
    word_t val2;

    while (len > 1) {
	val = *src++;
    val2 = *src++;
	*dst++ = val;
    *dst++ = val2;
	if (val > 0)
	    count++;
    if (val2 > 0)
        count++;
	len-=2;
    }
    while (len>0){
        val = *src;
        *dst = val;
        if (val>0)
        count++;
        len--;
    }
    return count;
}
