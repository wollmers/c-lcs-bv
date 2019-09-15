#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <nmmintrin.h>


typedef struct {
    void **keys;
    uint32_t *lens;
    uint64_t *bits;
} Hash;

typedef struct {
    uint32_t *ikeys;
    uint32_t *lens;
    uint64_t *bits;
} Hashi;

// calloc
void hash_new (Hash *hash, int size) {
    hash->keys = calloc(size+1, 
    	sizeof (void *)+sizeof (uint32_t *)+sizeof (uint64_t *));
    hash->lens = (uint32_t *)hash->keys + ((size+1) * sizeof (void *));
    hash->bits = (uint64_t *)hash->lens + ((size+1) * sizeof (uint32_t *));
}

// alloca
void hash_new_a (Hash *hash, int alen) {
    uint32_t size = (alen+1) * (
       sizeof (void *)+sizeof (uint32_t *)+sizeof (uint64_t *)
    );
    hash->keys = alloca(size);
    memset(hash->keys, 0, size);

    hash->lens = (uint32_t *)hash->keys + ((alen+1) * sizeof (void *));
    hash->bits = (uint64_t *)hash->lens + ((alen+1) * sizeof (uint32_t *));
}

void hash_destroy (Hash *hash) {
    free (hash->keys);
}
 
// memcmp; insert or return 
int hash_index (Hash *hash, char *key, uint32_t keylen) {
    int index = 0;
    while (hash->keys[index] 
           && ((uint32_t)hash->lens[index] != keylen 
             || memcmp(key, hash->keys[index], keylen))) {
        index++;
    }
    return index;
} 

int hashi_index (Hashi *hashi, uint32_t key) {
    int index = 0;
    while ( hashi->ikeys[index]
           && ((uint32_t)hashi->ikeys[index] != key) ) {
        index++;
    }
    return index;
}
 
// update bitmap
void hash_setpos (Hash *hash, void *key, uint32_t pos, uint32_t keylen) {
    int index = hash_index(hash, key, keylen);
    if (hash->keys[index] == 0) {
      	hash->keys[index] = key;
      	hash->lens[index] = keylen;
    }
    hash->bits[index] |= 1 << (pos % 64);
}

void hashi_setpos (Hashi *hashi, uint32_t key, uint32_t pos) {
    int index = hashi_index(hashi, key);
    if (hashi->ikeys[index] == 0) {
      	hashi->ikeys[index] = key;
    }
    hashi->bits[index] |= 1 << (pos % 64);
}
 
// get position bitmap
uint64_t hash_getpos (Hash *hash, void *key, uint32_t keylen) {
    int index = hash_index(hash, key, keylen);
    return hash->bits[index];
}

// get position bitmap
uint64_t hashi_getpos (Hashi *hashi, uint32_t key) {
    int index = hashi_index(hashi, key);
    return hashi->bits[index];
}

void hash_debug (Hash *hash, char *desc) {
//void hash_debug (Hash *hash) {
    uint32_t numEntries = 0;
    
    while (hash->keys[numEntries] ) {
        numEntries++;
    }
 
    printf ("=====: %s %d entries\n", desc, numEntries);
    //printf ("=====: %s %d entries\n", "lcs_a", numEntries);
 
    for (uint32_t i = 0; i < numEntries; i++) {
        printf ("Entry #%3d:", i);
        printf ("'%s'"," [");
        size_t j;
		for (j = 0; j < hash->lens[i]; j++) {
  		    printf("%.1s", (char *)hash->keys[i] + j);
		}
        printf ("'%s'"," = ");
  		printf("%llu", hash->bits[i] );
        printf ("'%s'\n","]");

    }
    //printf ("\n");
}

int count_bits(uint64_t bits) {
    bits = (bits & 0x5555555555555555ull) + ((bits & 0xaaaaaaaaaaaaaaaaull) >> 1);
    bits = (bits & 0x3333333333333333ull) + ((bits & 0xccccccccccccccccull) >> 2);
    bits = (bits & 0x0f0f0f0f0f0f0f0full) + ((bits & 0xf0f0f0f0f0f0f0f0ull) >> 4);
    bits = (bits & 0x00ff00ff00ff00ffull) + ((bits & 0xff00ff00ff00ff00ull) >> 8);
    bits = (bits & 0x0000ffff0000ffffull) + ((bits & 0xffff0000ffff0000ull) >>16);
    return (bits & 0x00000000ffffffffull) + ((bits & 0xffffffff00000000ull) >>32);
}

static const char allBytesForUTF8[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,5,5,5,5,6,6,6,6
};

// use stack for hash
int llcs_asci_a (char * a, char * b) {
    uint32_t alen = strlen(a);
    
    Hash hash;
    //hash_new(&hash, alen);

    // *** ugly begin ***
    void *keys[alen+1];
    uint32_t lens[alen+1];
    uint64_t bits[alen+1];
    hash.keys = keys;
    hash.lens = lens;
    hash.bits = bits;

    uint32_t i;
    for (i=0;i<=alen;i++) { 
      hash.keys[i] = 0;
      hash.lens[i] = 0;
      hash.bits[i] = 0;      
    }
    // *** ugly end ***
    
    for (i=0; *(a+i) != '\0'; i++){
      	hash_setpos (&hash, a+i, i, 1);
    }    

    //hash_debug (&hash, "seq_a");

    uint64_t v = ~0ull;

    uint32_t j;

    for (j=0; *(b+j) != '\0'; j++){
      uint64_t p = hash_getpos (&hash, b+j, 1);      
      uint64_t u = v & p;
      v = (v + u) | (v - u);
    }
    
    //return count_bits(~v);
    //return __builtin_popcountll(~v); // portable
    return _mm_popcnt_u64(~v);
}


// use stack for hash
int llcs_utf8_a (char * a, char * b) {
    uint32_t alen = strlen(a);
    uint32_t blen = strlen(b);
    
    Hash hash;
    //hash_new(&hash, alen);
    void *keys[alen+1];
    uint32_t lens[alen+1];
    uint64_t bits[alen+1];
    hash.keys = keys;
    hash.lens = lens;
    hash.bits = bits;

    uint32_t i;
    for (i=0;i<=alen;i++) { 
      hash.keys[i] = 0;
      hash.lens[i] = 0;
      hash.bits[i] = 0;      
    }

    uint32_t q, keylen;
    //for (i=0,q=0; (unsigned char)a[q] != '\0'; i++,q+=keylen){
    for (i=0,q=0; q < alen; i++,q+=keylen){
        keylen = allBytesForUTF8[(unsigned int)(unsigned char)a[q]];
      	hash_setpos (&hash, a+q, i, keylen);
      	//printf("setbit for char: %d %d %d %0llx\n", a[q],q,keylen,i);
    }     
    
    //hash_debug (&hash, "seq_utf8_a");
    
    uint64_t v = ~0ull;

    //for (q=0; (unsigned char)b[q] != '\0'; q+=keylen){
    for (q=0; q < blen; q+=keylen){
      keylen = allBytesForUTF8[(unsigned int)(unsigned char)b[q]]; 
      uint64_t p = hash_getpos (&hash, b+q, keylen);      
      //printf("posbit for char: %d %d %d %0llx\n", b[q],q,keylen, p);
      uint64_t u = v & p;
      v = (v + u) | (v - u);
    }

    //hash_destroy (&hash);
    return count_bits(~v);
}

// use stack for hash
int llcs_asci_i (char * a, char * b, uint32_t alen, uint32_t blen) {
    //uint32_t alen = strlen(a);
    
    Hashi hashi;
    //hash_new(&hash, alen);
    uint32_t ikeys[alen+1];
    uint32_t lens[alen+1];
    uint64_t bits[alen+1];
    hashi.ikeys = ikeys;
    hashi.lens = lens;
    hashi.bits = bits;

    int32_t i;
    for (i=0;i<=alen;i++) { 
      hashi.ikeys[i] = 0;
      hashi.lens[i] = 0;
      hashi.bits[i] = 0;      
    }
    
    //uint32_t q, keylen;
    uint32_t key;
    //for (i=0; *(a+i) != '\0'; i++){
    for (i=0,key=0; i < alen; i++){
      	//hash_setpos (&hash, a+i, i, 1);
      	//key = key << 8 | a[i];
      	key = a[i];
      	hashi_setpos (&hashi, key, i);
    }    

    //hash_debug (&hash, "seq_a");

    uint64_t v = ~0ull;

    uint32_t j;

    //for (j=0; *(b+j) != '\0'; j++){
    for (j=0,key=0; j < blen; j++){
      //uint64_t p = hash_getpos (&hash, b+j, 1);
      key = b[j];
      uint64_t p = hashi_getpos (&hashi, key);      
      uint64_t u = v & p;
      v = (v + u) | (v - u);
    }
    
    //return count_bits(~v);
    //return __builtin_popcountll(~v); // portable
    return _mm_popcnt_u64(~v);
}

// use utf-8 sequence as uint32_t key
int llcs_utf8_i (char * a, char * b, uint32_t alen, uint32_t blen) {
    //uint32_t alen = strlen(a);
    //uint32_t blen = strlen(b);
    
    Hashi hashi;
    //hash_new(&hash, alen);
    uint32_t ikeys[alen+1];
    uint32_t lens[alen+1];
    uint64_t bits[alen+1];
    hashi.ikeys = ikeys;
    hashi.lens = lens;
    hashi.bits = bits;

    int32_t i;
    for (i=0;i<=alen;i++) { 
      hashi.ikeys[i] = 0;
      hashi.lens[i] = 0;
      hashi.bits[i] = 0;      
    }

    uint32_t q, keylen;
    uint32_t key, k;
    //for (i=0,q=0; (unsigned char)a[q] != '\0'; i++,q+=keylen){
    for (i=0,q=0; q < alen; i++,q+=keylen){
        keylen = allBytesForUTF8[(unsigned int)(unsigned char)a[q]];
        for (k=0,key=0; k < keylen; k++) {
          key = key << 8 | a[q+k];
        }
      	hashi_setpos (&hashi, key, i);
      	//printf("setbit for char: %d %d %d %0llx\n", a[q],q,keylen,i);
    }
    
    //hash_debug (&hash, "seq_utf8_a");
    
    uint64_t v = ~0ull;
    int32_t j;
    //uint64_t Vs[blen+1];

    //for (q=0; (unsigned char)b[q] != '\0'; q+=keylen){
    for (j=0,q=0; q < blen; j++,q+=keylen){
      keylen = allBytesForUTF8[(unsigned int)(unsigned char)b[q]];
        for (k=0,key=0; k < keylen; k++) {
          key = key << 8 | b[q+k];
        }      
       
      uint64_t p = hashi_getpos (&hashi, key);      
      //printf("posbit for char: %d %d %d %0llx\n", b[q],q,keylen, p);
      uint64_t u = v & p;
      v = (v + u) | (v - u);
    }

    return count_bits(~v);
    //return _mm_popcnt_u64(~v);
}

/* ######### LCS ################ */

int lcs_seq_utf8_a (char * a, char * b, int32_t (* lcs)[2]) {
    uint32_t alen = strlen(a);
    uint32_t blen = strlen(b);
    
    Hash hash;
    //hash_new(&hash, alen);
    void *keys[alen+1];
    uint32_t lens[alen+1];
    uint64_t bits[alen+1];
    hash.keys = keys;
    hash.lens = lens;
    hash.bits = bits;

    int32_t i;
    for (i=0;i<=alen;i++) { 
      hash.keys[i] = 0;
      hash.lens[i] = 0;
      hash.bits[i] = 0;      
    }

    uint32_t q, keylen;
    //for (i=0,q=0; (unsigned char)a[q] != '\0'; i++,q+=keylen){
    for (i=0,q=0; q < alen; i++,q+=keylen){
        keylen = allBytesForUTF8[(unsigned int)(unsigned char)a[q]];
      	hash_setpos (&hash, a+q, i, keylen);
      	//printf("setbit for char: %d %d %d %0llx\n", a[q],q,keylen,i);
    }     
    
    //hash_debug (&hash, "seq_utf8_a");
    
    uint64_t v = ~0ull;
    int32_t j;
    uint64_t Vs[blen+1];

    //for (q=0; (unsigned char)b[q] != '\0'; q+=keylen){
    for (j=0,q=0; q < blen; j++,q+=keylen){
      keylen = allBytesForUTF8[(unsigned int)(unsigned char)b[q]];     
       
      uint64_t p = hash_getpos (&hash, b+q, keylen);      
      //printf("posbit for char: %d %d %d %0llx\n", b[q],q,keylen, p);
      uint64_t u = v & p;
      v = (v + u) | (v - u);
      Vs[j] = v;
    }

    int llcs = count_bits(~v)-1;
    
    // i = amax;
    // j = bmax;
    //printf("i: %d \n", i);
    //printf("j: %d \n", j);
    //printf("llcs: %d \n", llcs);
    i--;
    j--;
    
    //uint32_t k;
    //uint32_t width;

    while (i >= 0 && j >= 0) {
      //printf("i: %d j: %d llcs: %d\n", i, j, llcs);
      if (Vs[j] & (1<<i)) {
        i--;
      }
      else {
        if ( j && ~Vs[j-1] & (1<<i) ) { }
        else {
           lcs[llcs][0] = i;
           lcs[llcs][1] = j;
           i--;
           llcs--;
        }
        j--;
      }
    }
    return count_bits(~v);
}


/*
if (SvUTF8 (sv))
    {
       STRLEN clen;
       while (len)
         {
           *p++ = utf8n_to_uvchr (s, len, &clen, 0);

           if (clen < 0)
             croak ("illegal unicode character in string");

           s += clen;
           len -= clen;
         }
    }
  else
    while (len--)
      *p++ = *(unsigned char *)s++;

  *lenp = p - r;
  return r;
*/
 
int main (void) {
    clock_t tic;
    clock_t toc;
    double elapsed;
    double rate;
    
    uint32_t count;
    uint32_t iters = 3 * 1000000;
    //uint32_t iters = 1;
    
    char str1[] = "Choerephon";
    char str2[] = "Chrerrplzon";
    
    //char str1[] = "Chöerephon";
    //char str2[] = "Chöerrplzon";
    
    //char str1[] = "coult n0t creAte has";
    //char str2[] = "Could not create has";
    
    //char str1[] = "Could not create hash pfHashTable *tbl =";
    //char str1[] = "Could not create hash pfHashTable *tbl = pfHashCreate (NULL)";

    
    //char str1[] = "used";
    //char str2[] = "sued";
    
    uint32_t len1 = strlen(str1);
    uint32_t len2 = strlen(str2);

    
    int length_lcs;
    /* ################### */

    printf("llcs_asci_a - ascii, stack,  sequential addressing\n");
    printf("llcs_asci_i - ascii, stack,  sequential addressing, store key\n");
    printf("llcs_utf8_a - utf8, stack, sequential addressing\n");
    printf("llcs_utf8_i - utf8, stack, sequential addressing, store key\n");


    /* ########## llcs_asci_a ########## */
    
  	for (count = 0; count < 1; count++) {
      	length_lcs = llcs_asci_a (str1, str2);
      	//printf("llcs_a - ascii, stack, sequential addressing\n");
      	//printf("llcs_a: %d\n", length_lcs);
  	}
     
    tic = clock();
    
  	for (count = 0; count < iters; count++) {
      	length_lcs = llcs_asci_a (str1, str2);
  	}

    toc = clock();
    elapsed = (double)(toc - tic) / CLOCKS_PER_SEC;
    rate    = (double)iters / elapsed;
    printf("[llcs_a]      Elapsed: %f seconds Rate: %f (1/sec)\n", elapsed, rate);

    /* ######### llcs_asci_i ########### */
    
  	for (count = 0; count < 1; count++) {
      	length_lcs = llcs_asci_i (str1, str2, len1, len2);
      	//printf("llcs_a - ascii, stack, sequential addressing\n");
      	//printf("llcs_a: %d\n", length_lcs);
  	}
     
    tic = clock();
    
  	for (count = 0; count < iters; count++) {
      	length_lcs = llcs_asci_i (str1, str2, len1, len2);
  	}

    toc = clock();
    elapsed = (double)(toc - tic) / CLOCKS_PER_SEC;
    rate    = (double)iters / elapsed;
    printf("[llcs_i]      Elapsed: %f seconds Rate: %f (1/sec)\n", elapsed, rate);
    
    /* ########## llcs_utf8_a ########## */

  
  	for (count = 0; count < 1; count++) {
      	length_lcs = llcs_utf8_a (str1, str2);
      	//printf("llcs - utf8, stack, sequential addressing\n");
      	printf("llcs_utf8_a: %d\n", length_lcs);
  	}
     
    tic = clock();
    
  	for (count = 0; count < iters; count++) {
      	length_lcs = llcs_utf8_a (str1, str2);
  	}

    toc = clock();
    elapsed = (double)(toc - tic) / CLOCKS_PER_SEC;
    rate    = (double)iters / elapsed;
    printf("[llcs_utf8_a] Elapsed: %f seconds Rate: %f (1/sec)\n", elapsed, rate);
    
    /* ########## llcs_utf8_i ########## */

  
  	for (count = 0; count < 1; count++) {
      	//length_lcs = llcs_seq_utf8_i (str1, str2);
      	length_lcs = llcs_utf8_i (str1, str2, len1, len2);
      	//printf("llcs - utf8, stack, sequential addressing\n");
      	printf("llcs_utf8_i: %d\n", length_lcs);
  	}
     
    tic = clock();
    
  	for (count = 0; count < iters; count++) {
      	//length_lcs = llcs_seq_utf8_i (str1, str2);
      	length_lcs = llcs_utf8_i (str1, str2, len1, len2);
  	}

    toc = clock();
    elapsed = (double)(toc - tic) / CLOCKS_PER_SEC;
    rate    = (double)iters / elapsed;
    printf("[llcs_utf8_i] Elapsed: %f seconds Rate: %f (1/sec)\n", elapsed, rate);

    /* ######## LCS ############ */
    
  	    uint32_t len_a = strlen(str1);
  	    int32_t lcs[len_a][2]; 
  	    
  	for (count = 0; count < 1; count++) {

  	    lcs[0][0] = -1;
  	    lcs[0][1] = -1;
  	    
      	length_lcs = lcs_seq_utf8_a (str1, str2, lcs);
      	printf("lcs_utf8_a: %d\n", length_lcs);
      	uint32_t col;
      	for (col = 0; col < length_lcs; col++) {
      	    printf("%d\t", lcs[col][0]);
      	    
      	}
      	printf("\n");
       	for (col = 0; col < length_lcs; col++) {
      	    printf("%d\t", lcs[col][1]);
      	    
      	} 
      	printf("\n");    	
  	}
     
    tic = clock();
    
  	for (count = 0; count < iters; count++) {

  	    lcs[0][0] = -1;
  	    lcs[0][1] = -1;
  	    
      	length_lcs = lcs_seq_utf8_a (str1, str2, lcs);
  	}

    toc = clock();
    elapsed = (double)(toc - tic) / CLOCKS_PER_SEC;
    rate    = (double)iters / elapsed;
    printf("[lcs_utf8_a] Elapsed: %f seconds Rate: %f (1/sec)\n", elapsed, rate);

    /* #################### */                  
    return 0;

}
