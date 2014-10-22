//-- /* BEGIN INCLUDE FILE area_structures.incl.pl1  10/75 */
//-- /* Modified September 1981 by J. Bongiovanni for allocation_p_clock */
//-- 
//-- dcl  area_version_1 fixed bin static init (1);             /* version number for this area format */
//-- 
//-- dcl  areap ptr;
//-- 
//-- dcl 1 area_header aligned based (areap),
//--     2 version fixed bin (35),                              /* 0 for buddy system, 1 for current areas */
//--     2 last_usable bit (18),                                /* rel pointer to end of area */
//--     2 next_virgin bit (18),                                /* rel pointer to next slot in virgin territory */
//--     2 flags,
//--       3 extend bit (1) unal,                               /* says area is extensible */
//--       3 zero_on_alloc bit (1) unal,                        /* says to zero after allocation */
//--       3 zero_on_free bit (1) unal,                 /* says to zero after freeing */
//--       3 dont_free bit (1) unal,                    /* dont honor free request -- debugging tool */
//--       3 defined_by_call bit (1) unal,                      /* says area seg got via define_area_ call */
//--       3 system bit (1) unal,                               /* ayss area is managed by the system */
//--       3 mbz bit (30) unal,
//--     2 allocation_method fixed bin,                 /* 0 is standard, 1 is no_freeing method */
//--     2 last_size bit (18),                          /* size of last allocated block before virgin territory */
//--     2 last_block bit (18),                         /* rel pointer to last allocated block before virgin territory */
//--     2 freep (3:16),                                        /* free list info */
//--       3 relp bit (18) unal,                                /* pointer to first block on list */
//--       3 max_block_size bit (18) unal,                      /* size of largest block on list, if known. else zero */
//--     2 allocation_p_clock bit (36) aligned,         /* counter to prevent IPS race */
//--     2 extend_info bit (18) unal,                   /* offset to extend info block */
//--     2 recovery_info bit (18) unal,                 /* eventually will hold recovery info relp */
//--     2 n_allocated fixed bin (17) unal,                     /* number of allocated blocks */
//--     2 n_free fixed bin (17) unal;                  /* number of free blocks */
 
typedef struct __attribute__ ((__packed__)) area_header
  {
    word36s version; /* 0 for buddy system, 1 for current areas */
    union
      {
         struct __attribute__ ((__packed__))
          {
            uint next_virgin : 18;/* rel pointer to next slot in virgin territory */
            uint last_usable : 18; /* rel pointer to end of area */
          };
        word36 align1;
      };
    union
      {
         struct __attribute__ ((__packed__))
          {
            uint mbz : 30;
            uint system : 1; /* ayss area is managed by the system */
            uint defined_by_call : 1; /* says area seg got via define_area_ call */
            uint dont_free : 1; /* dont honor free request -- debugging tool */
            uint zero_on_free : 1; /* says to zero after freeing */
            uint zero_on_alloc : 1; /* says to zero after allocation */
            uint extend : 1; /* says area is extensible */
          };
        word36 align2;
      };
    word36s allocation_method; /* 0 is standard, 1 is no_freeing method */
    union
      {
         struct __attribute__ ((__packed__))
          {
            uint last_block : 18; /* rel pointer to last allocated block before virgin territory */
            uint last_size : 18; /* size of last allocated block before virgin territory */
          };
        word36 align3;
      };
    union
      {
         struct __attribute__ ((__packed__))
          {
            uint max_block_size : 18; /* size of largest block on list, if known. else zero */
            uint relp : 18; /* pointer to first block on list */
          };
        word36 align4;
      } freep [14]; // [3..16]
    word36 allocation_p_clock; /* counter to prevent IPS race */
    union
      {
         struct __attribute__ ((__packed__))
          {
            uint recovery_info : 18; /* eventually will hold recovery info relp */
            uint extend_info : 18; /* offset to extend info block */
          };
        word36 align5;
      };
    union
      {
         struct __attribute__ ((__packed__))
          {
            int n_free : 18; /* number of free blocks */
            int n_allocated : 18; /* number of allocated blocks */
          };
        word36 align6;
      };
  } area_header;

//-- dcl (STANDARD_ALLOCATION_METHOD init (0),
//--      NO_FREEING_ALLOCATION_METHOD init (1)
//--      ) fixed bin internal static;

#define STANDARD_ALLOCATION_METHOD 0
#define NO_FREEING_ALLOCATION_METHOD 1

//-- dcl  extend_blockp ptr;
//-- 
//-- dcl 1 extend_block aligned based (extend_blockp),          /* contents of extend block for extensible areas */
//--     2 first_area ptr unal,                         /* pointer to first area */
//--     2 next_area ptr unal,                          /* pointer to next area in chain */
//--     2 sequence_no fixed bin,                               /* sequence number for this component */
//--     2 name char (32),                              /* owner of the area */
//--     2 pad fixed;                                   /* brings it to an even, 12 word allocation */
//-- 
//-- dcl alloc_blkhdrsz static internal init(2);
//-- dcl 1 no_free_area_header aligned based(areap),            /* overlay for no_free areas */
//--     2 pad(4) ptr,
//--     2 current_component ptr;                               /* points to component from which we are allocating */
//-- 
//-- dcl  blockp ptr;
//-- 
//-- dcl 1 block aligned based (blockp),                        /* declaration for block header */
//--     2 prev_size bit (18) unal,                     /* size of preceding block */
//--     2 cur_size bit (18) unal,                              /* size of current block */
//--     2 buddy_pad bit (8) unal,                              /* non_zero for buddy system area */
//--     2 prev_busy bit (1) unal,                              /* previous-block-is-used flag */
//--     2 marked bit (1) unal,
//--     2 q_no bit (8) unal,                           /* stratum number when in free list */
//--     2 header bit (18) unal,                                /* pointer to head of area */
//--     2 fp bit (18) unal,                            /* forward free list thread */
//--     2 bp bit (18) unal;                            /* backward free list thread */
//-- 
//-- dcl  min_block_size fixed bin static init (8);             /* minimum allowed block size */
//-- 
//-- /* END INCLUDE FILE area_structures.incl.pl1 */
