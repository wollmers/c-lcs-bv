#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    void **keys;
    uint32_t *lens;
    uint64_t *bits;
} Hash;

void hash_new (Hash *hash, int size) {
    hash->keys = calloc(size+1, 
    	sizeof (void *)+sizeof (uint32_t *)+sizeof (uint64_t *));
    hash->lens = (uint32_t *)hash->keys + ((size+1) * sizeof (void *));
    hash->bits = (uint64_t *)hash->lens + ((size+1) * sizeof (uint32_t *));
}
 
int hash_index (Hash *hash, char *key, uint32_t keylen) {
    int index = 0;
    while (hash->keys[index] 
           && ((uint32_t)hash->lens[index] != keylen 
             || memcmp(key, hash->keys[index], keylen))) {
        index++;
    }
    return index;
} 
 
void hash_setpos (Hash *hash, void *key, uint32_t pos, uint32_t keylen) {
    int index = hash_index(hash, key, keylen);
    if (hash->keys[index] == 0) {
      	hash->keys[index] = key;
      	hash->lens[index] = keylen;
    }
    hash->bits[index] |= 1 << (pos % 64);
}
 
uint64_t hash_getpos (Hash *hash, void *key, uint32_t keylen) {
    int index = hash_index(hash, key, keylen);
    return hash->bits[index];
}

void hash_destroy (Hash *hash) {
    free (hash->keys);
}

void hash_debug (Hash *hash, char *desc) {

    uint32_t numEntries = 0;
    
    while (hash->keys[numEntries] ) {
        numEntries++;
    }
 
    printf ("=====: %s %d entries\n", desc, numEntries);
 
    // For each lookup entry, free the list.
 
    for (uint32_t i = 0; i < numEntries; i++) {
        // Only output if there are nodes.
        if (hash->keys[i] != NULL) {
            printf ("Entry #%3d:", i);
                printf ("'%s'"," [");
                size_t j;
				for (j = 0; j < hash->lens[i]; j++) {
  				  printf("%.1s", (char *)hash->keys[i] + j);
				}
                printf ("'%s'"," = ");
				for (j = 0; j < hash->lens[i]; j++) {
  				  //printf("%.1s", (char *)hash->values[i] + j);
				}
                printf ("'%s'\n","]");
        }
    }
    printf ("\n");
}

int count_bits(uint64_t bits) {
  bits = (bits & 0x5555555555555555ull) + ((bits & 0xaaaaaaaaaaaaaaaaull) >> 1);
  bits = (bits & 0x3333333333333333ull) + ((bits & 0xccccccccccccccccull) >> 2);
  bits = (bits & 0x0f0f0f0f0f0f0f0full) + ((bits & 0xf0f0f0f0f0f0f0f0ull) >> 4);
  bits = (bits & 0x00ff00ff00ff00ffull) + ((bits & 0xff00ff00ff00ff00ull) >> 8);
  bits = (bits & 0x0000ffff0000ffffull) + ((bits & 0xffff0000ffff0000ull) >>16);
  return (bits & 0x00000000ffffffffull) + ((bits & 0xffffffff00000000ull) >>32);
}

int llcs_seq (char * a, char * b) {
    uint32_t alen = strlen(a);
    
    //Hash *hash = hash_new(alen);
    Hash hash;
    hash_new(&hash, alen);

    uint32_t i;
    for (i=0; *(a+i) != '\0'; i++){
      	hash_setpos (&hash, a+i, i, 1);
    }     
    
    uint64_t v = ~0ull;

    uint32_t j;

    for (j=0; *(b+j) != '\0'; j++){
      uint64_t p = hash_getpos (&hash, b+j, 1);      
      //printf("posbit for char: %d %0llx\n", j, p);
      uint64_t u = v & p;
      v = (v + u) | (v - u);
    }

    hash_destroy (&hash);
    return count_bits(~v);
}

 
int main (void) {
    clock_t tic;
    clock_t toc;
    double elapsed;
    double rate;
    
    uint32_t count;
    uint32_t iters = 3 * 1000000;
    //uint32_t iters = 1;
    
    char str1[] = "Choerephon";
    char str2[] = "Choerrplzon";
    
    //char str1[] = "coult n0t creAte has";
    //char str2[] = "Could not create has";
    
    //char str1[] = "Could not create hash pfHashTable *tbl =";
    //char str1[] = "Could not create hash pfHashTable *tbl = pfHashCreate (NULL)";

    
    //char str1[] = "used";
    //char str2[] = "sued";

    /* ################### */
    
    
    int length_lcs;
    
  	for (count = 0; count < 1; count++) {
      	length_lcs = llcs_seq (str1, str2);
      	printf("llcs: %d\n", length_lcs);
  	}
     
    tic = clock();
    
  	for (count = 0; count < iters; count++) {
      	length_lcs = llcs_seq (str1, str2);
  	}

    toc = clock();
    elapsed = (double)(toc - tic) / CLOCKS_PER_SEC;
    rate    = (double)iters / elapsed;
    printf("[llcs] Elapsed: %f seconds Rate: %f (1/sec)\n", elapsed, rate);
    
    /* #################### */
        
    return 0;

}