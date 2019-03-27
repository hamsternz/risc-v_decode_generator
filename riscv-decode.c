#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

static char entity_name[] = "decoder";
static char arch_name[] = "auto_generated_code";
static char inst_name[] = "inst";
char buffer[1024];
int mode;
#define MODE_SLICES 1
#define MODE_FLAGS  2

struct Slice {
  struct Slice *next;
  char *name;
  uint32_t width;
  uint8_t *indexes;
};

struct Product {
  struct Product *next;
  uint32_t mask;
  uint32_t value;
};

struct Flag {
  struct Flag *next;
  char *name;
  struct Product *first_product;
  struct Product *first_type_product;
};

struct Flag_List {
  struct Flag_List *next;
  struct Flag *flag;
};

struct Pattern {
  struct Pattern *next;
  uint32_t mask;
  uint32_t value;
  struct Flag_List *flag_list;
};

struct Slice   *first_slice;
struct Pattern *first_pattern;
struct Flag    *first_flag;

/****************************************************************************/
int symbol_first_char(char c) {
  if(c >= 'a' && c <= 'z') return 1;
  if(c >= 'A' && c <= 'Z') return 1;
  return 0;
}
/****************************************************************************/
int symbol_char(char c) {
  if(c >= 'a' && c <= 'z') return 1;
  if(c >= 'A' && c <= 'Z') return 1;
  if(c >= '0' && c <= '9') return 1;
  if(c == '_') return 1;
  return 0;
}
/****************************************************************************/
int read_line(FILE *f) {
  int buffer_index = 0;
  int c;
  while(1) {
    c = getc(f);
    if(c == EOF) {
      /* Only finish if the buffer is empty */
      if(buffer_index == 0) {
	return 0;
      } else {
	buffer[buffer_index] = '\0';
	return 1;
      }
    } else if(c == '\r') {
      /* Do nothing */
    } else if(c == '\n') {
      buffer[buffer_index] = '\0';
      return 1;
    } else {
      /* Add to buffer */
      buffer[buffer_index] = c;
      buffer_index++;
    }
  }
  return 1;
}

/****************************************************************************/
struct Product *field_add_product(struct Flag *f, uint32_t mask, uint32_t value) {
  struct Product *p;

  // Allocate new item
  p = malloc(sizeof(struct Product));
  if(p == NULL) {
    fprintf(stderr,"Out of memory\n");
    return NULL;
  }
  memset(p,0,sizeof(struct Product));

  // Set the values 
  p->mask  = mask;
  p->value = value;

  // Add to the list
  p->next  = f->first_product;
  f->first_product = p;
  return p;
}

/****************************************************************************/
struct Product *field_add_type_product(struct Flag *f, uint32_t mask, uint32_t value) {
  struct Product *p;
  p = f->first_type_product;
  while(p != NULL) {
    if(p->mask == mask && p->value == value)
      return p;
    p = p->next;
  }
  // Allocate new item
  p = malloc(sizeof(struct Product));
  if(p == NULL) {
    fprintf(stderr,"Out of memory\n");
    return NULL;
  }
  memset(p,0,sizeof(struct Product));

  // Set the values 
  p->mask  = mask;
  p->value = value;

  // Add to the head of the list
  p->next  = f->first_type_product;
  f->first_type_product = p;
  return p;
}

/****************************************************************************/
struct Flag *find_flag(char *name, int len) {
  struct Flag *f = first_flag;
  while(f != NULL) {
    int i;

    for(i = 0; i < len; i++) {
      if(name[i] != f->name[i]) 
        break;
    }

    if(f->name[i] == '\0' && (name[i] == '\0' || name[i] == ' ' || name[i] == '\t'))
      break;

    f = f->next;
  }
  return f;
}
/****************************************************************************/
struct Flag *add_flag(char *name, int len) {
  struct Flag *f;
  f = malloc(sizeof(struct Flag));
  if(f == NULL) {
    fprintf(stderr, "Out of memory\n");
    return NULL;
  }
  memset(f,0,sizeof(struct Flag));
  f->name = malloc(len+1);
  if(f->name == NULL) {
    free(f);
    fprintf(stderr, "Out of memory\n");
    return NULL;
  }
  memcpy(f->name,name,len);
  f->name[len] = 0;

  if(first_flag == NULL || strcmp(f->name, first_flag->name) < 0) { 
    // Insert at head of list
    f->next = first_flag; 
    first_flag = f;
  } else { 
    struct Flag *c = first_flag;
    // find where we need to insert it
    while(c->next != NULL && strcmp(f->name, c->next->name) > 0) {
      c = c->next;
    }
    // Insert into list
    f->next = c->next;
    c->next = f;
  }
  return f;
}
/****************************************************************************/
struct Pattern *add_pattern(uint32_t mask, uint32_t value, struct Flag_List *flag_list) {
  struct Pattern *p;
  p = malloc(sizeof(struct Pattern));
  if(p == NULL) {
    fprintf(stderr, "Out of memory\n");
    return NULL;
  }
  memset(p,0,sizeof(struct Pattern));
  p->mask = mask;
  p->value = value;
  p->flag_list = flag_list;

  if(first_pattern == NULL) {
    first_pattern = p;
  } else {
    // Add at end of list
    struct Pattern *c = first_pattern;
    while(c->next != NULL) {
      c = c->next;
    }
    c->next = p;
  }
  return p;
}

/****************************************************************************/
struct Slice *add_slice(char *name, int len, uint8_t width) {
  struct Slice *s;
  s = malloc(sizeof(struct Slice));
  if(s == NULL) {
    fprintf(stderr, "Out of memory\n");
    return NULL;
  }
  memset(s,0,sizeof(struct Slice));
  s->name  = malloc(len+1);
  s->width = width;
  if(s->name == NULL) {
    free(s);
    fprintf(stderr, "Out of memory\n");
    return NULL;
  }
  memcpy(s->name,name,len);
  s->name[len] = 0;

  s->indexes = malloc(sizeof(uint8_t) * width);
  if(s->indexes == NULL) {
    fprintf(stderr, "Out of memory\n");
    free(s->name);
    free(s);
    return NULL;
  }
  memset(s->indexes, 0xFF, sizeof(uint8_t) * width);

  if(first_slice == NULL || strcmp(s->name, first_slice->name) < 0) { 
    // Insert at head of list
    s->next = first_slice; 
    first_slice = s;
  } else { 
    struct Slice *c = first_slice;
    // find where we need to insert it
    while(c->next != NULL && strcmp(s->name, c->next->name) > 0) {
      c = c->next;
    }
    // Insert into list
    s->next = c->next;
    c->next = s;
  }
  return s;
}
/****************************************************************************/
int parse_line(void) {
  char *ptr = buffer;
  int i=0;
  uint32_t mask, value;
  struct Flag      *flag        = NULL;
  struct Flag_List *flag_list   = NULL;
  struct Pattern   *new_pattern = NULL;

  while(*ptr == ' ')
     ptr++;

  if(*ptr == '#' || *ptr == '\0')
    return 1;

  if(strncmp(ptr,"Slices:",7) == 0) {
    mode = MODE_SLICES;
    return 1;
  }
  if(strncmp(ptr,"Flags:",6) == 0) {
    mode = MODE_FLAGS;
    return 1;
  }

  switch(mode) {
    case MODE_FLAGS:
      for(i = 0; i < 32; i++) {
        if(ptr[i] != '0' && ptr[i] != '1' && ptr[i] != '-') {
          fprintf(stderr, "Expected a 32-bit long mask\n");
          return 0;
        }
      }
    
      if(ptr[32] == '0' || ptr[i] == '1' || ptr[i] == '-') {
        fprintf(stderr, "Expected a 32-bit long mask\n");
        return 0;
      }
    
      mask = value = 0;
      for(i = 0; i < 32; i++) {
        switch(ptr[i]) {
          case '-':
	    break;
          case '0':
	    mask |= 1<<(31-i);
	    break;
          case '1':
            value |= 1<<(31-i);
	    mask  |= 1<<(31-i);
	    break;
        }
      }
      ptr += 32;

      // Look for '[whitepace]=[whitepages]' or the end of line
      while(*ptr == ' ')
        ptr++;

      if(*ptr == '\0' || *ptr == '#') {
	// No flags to be set but it is a valid opcode
        new_pattern = add_pattern(mask,value, flag_list);
        if(new_pattern == NULL) {
          fprintf(stderr,"Unable to create new Pattern\n");
          return 0;
        }
        return 1;
      }

      if(*ptr != '=') {
        fprintf(stderr,"Expected an '=' sign\n");
        return 0;
      }
      ptr++;

      while(*ptr == ' ')
        ptr++;
  
      while(1) { 
        struct Flag_List *new_entry;
	int len = 0;

	if(!symbol_first_char(ptr[len])) {
          fprintf(stderr,"Invalid Symbol character '%c'\n", ptr[len]);
          return 0;
	}
	len++;
	while(symbol_char(ptr[len]))
	   len++;

        // Now extract the list of flags that will be set for this line
        flag = find_flag(ptr,len);
        if(flag == NULL) {
          flag = add_flag(ptr,len);
        }
        if(flag == NULL) {
   	  fprintf(stderr,"Unable to add flag\n");
    	  return 0;
        }
	ptr += len;
 
	// Create new entry
        new_entry = malloc(sizeof(struct Flag_List));
        if(new_entry == NULL) {
          fprintf(stderr,"Out of memory\n");
          return 0;
        }
        memset(new_entry,0,sizeof(struct Flag_List));
        new_entry->flag =  flag;

	// Add to the list
	if(flag_list == NULL) {
	  // Empty list 
	  flag_list = new_entry;
	} else {
	  // Add to the end of the list 
          struct Flag_List *c = flag_list;
	  while(c->next != NULL)
	    c = c->next;
	  c->next = new_entry;
	}

        while(*ptr == ' ')
          ptr++;
	// End of list? 
	if(*ptr == '\0' || *ptr == '#')
	  break;

	// Should have comma
        if(*ptr != ',') {
          fprintf(stderr,"Expected comma seperate list\n");
          return 0;
	}
        ptr++;

        while(*ptr == ' ')
          ptr++;
      } 

      // Create the new pattern
      new_pattern = add_pattern(mask,value, flag_list);
      if(new_pattern == NULL) {
        free(flag_list);
        fprintf(stderr,"Unable to create new Pattern\n");
        return 0;
      }
      return 1;
    case MODE_SLICES:
      {
        int len = 0;
	int width = 1;
	struct Slice *s;

        if(!symbol_first_char(ptr[len])) {
          fprintf(stderr,"Invalid Symbol character '%c'\n", ptr[len]);
          return 0;
        }
        len++;
        while(symbol_char(ptr[len]))
          len++;
	ptr += len;

        while(*ptr == ' ')
	  ptr++;

	if(*ptr != '(') {
          fprintf(stderr,"Expected '(' got '%c'\n", *ptr);
          return 0;
	}
	ptr++;

        while(*ptr == ' ')
	   ptr++;
	   
	if((ptr[0] >= '1' && ptr[0] <= '9') && (ptr[1] >= '0' && ptr[1] <= '9')) {
	  width =  (ptr[0] - '0')*10 + ptr[1] - '0';
	  ptr += 2;
	} else if(ptr[0] >= '1' && ptr[0] <= '9') {
	  width =  ptr[0] - '0';
	  ptr++;
	} else {
          fprintf(stderr,"Expected a two-digit width\n");
          return 0;
	}

        while(*ptr == ' ')
	   ptr++;
	   
	if(*ptr != ')') {
          fprintf(stderr,"Expected ')'\n");
          return 0;
	}
	ptr++;
	
        while(*ptr == ' ')
	   ptr++;
	   
	if(*ptr != '=') {
          fprintf(stderr,"Expected '='\n");
          return 0;
	}
	ptr++;

        while(*ptr == ' ')
	   ptr++;
	  
        for(i = 0; i < 32; i++) {
	  if((ptr[i] < '0' && ptr[i] > '9') && (ptr[i] < 'a' && ptr[i] > 'z') && (ptr[i] < 'A' && ptr[i] > 'Z') && ptr[i] != '-') {
	    fprintf(stderr, "Expecting a 32-bit slice discription\n");
	    return 0;
	  }
	}	
	// eat whitespace
	while(ptr[i] == ' ')
	   i++;
	if(ptr[i] != '#' && ptr[i] != '\0') {
	    fprintf(stderr, "Expecting comment or end of line\n");
//	    return 0;
	}
	s = add_slice(buffer,len,width);
        for(i =0; i < 32; i++) {
	  int index;
	  if(ptr[i] >= '0' && ptr[i] <= '9') {
	    index = ptr[i] - '0';
	  } else if(ptr[i] >= 'a' && ptr[i] <= 'z') {
	    index = ptr[i] - 'a' + 10;
	  } else if(ptr[i] >= 'A' && ptr[i] <= 'Z') {
	    index = ptr[i] - 'A' + 10;
	  } else {
            index = 0xFF;
	  }


	  if(index != 0xFF) {
	    if(index >= width) {
	      fprintf(stderr, "Index '%c' (%i) is out of slice\n", ptr[i], index);
	      return 0;
	    } else if(s->indexes[index] != 0xFF) {
	      fprintf(stderr, "Index is assigned multiple times\n");
	      return 0;
	    } else {  
	      s->indexes[index] = 31-i;
	    }
	  }
	}
	
      }
      return 1;
    default:
      fprintf(stderr,"Not in 'Flags' or 'Fields' mode\n");
      return 0;
  }
}

/****************************************************************************/
int pattern_overlaps(struct Pattern *p1, struct Pattern *p2) {
  uint32_t common_mask;
  // If the bits that have assigned values do not overlap then
  // we have overlapping patterns 
  common_mask = p1->mask & p2->mask;
  if(common_mask == 0)
    return 1;

  // If all the bits where the masks overlap are the same
  // then we can again have overlapping patterns
  if((p1->value & common_mask) == (p2->value & common_mask))
    return 1;

  // If the assigned bits do not match, then we have no overlap
  return 0;
}

/****************************************************************************/
void print_product(FILE *f, struct Product *product) { 
  int i;

  for(i = 31; i >= 0; i--) {
    if(product->mask & (1<<i)) {
      if(product->value & (1<<i)) {
        putc('1',f);
      } else {
        putc('0',f);
      }
    } else {
      putc('-',f);
    }
  }
  fprintf(f, "\n");
}

/****************************************************************************/
void print_flag(FILE *f, struct Flag *flag) { 
  struct Product *p;
  fprintf(f,"--   %-20s ", flag->name);
  p = flag->first_product;
  while(p != NULL) {
    print_product(f, p);
    if(p->next != NULL) {
      fprintf(f, "--   %-18s + ", "");
    }
    p = p->next;
  }

  p = flag->first_type_product;
  if(p != NULL)
  {
    printf("--\n");
    printf("--              Type mask ");
  }
  while(p != NULL) {
    print_product(f, p);
    if(p->next != NULL) {
      fprintf(f, "--   %-18s + ", "");
    }
    p = p->next;
  }
  fprintf(f, "--\n");
  fprintf(f, "--\n");
}

/****************************************************************************/
int print_pattern(FILE *f, struct Pattern *p) {
  int i;
  // Binary dump 
  for(i = 31; i >= 0; i--) {
    if(p->mask & (1<<i)) {
      if(p->value & (1<<i)) {
        putc('1',f);
      } else {
        putc('0',f);
      }
    } else {
      putc('-',f);
    }
  }

  if(p->flag_list != NULL) {
    // List of flags to be set
    struct Flag_List *fl;
    fprintf(f, " = ");
    fl = p->flag_list;
    while(fl != NULL) {
      fprintf(f, "%s",fl->flag->name);
      if(fl->next != NULL)
        fprintf(f, ", ");
      fl = fl->next;
    }
  }
  fprintf(f, "\n");
  return 1;
}
/****************************************************************************/
int attempt_merge_inner(struct Product *p, uint32_t merge_bit) { 
  struct Product *p2;

  // Can't possibly merge this
  if((p->mask & merge_bit) == 0) return 0;
  
  // The bit is set in this mask, so let's see if we can find 
  // The other pattern to merge with 
  p2 = p->next;
  while(p2 != NULL) {
    if(p->mask == p2->mask) {
      if((p->value & merge_bit) != (p2->value & merge_bit) &&
         (p->value | merge_bit) == (p2->value | merge_bit)) {
        int merge_found = 0;
	struct Product *c;
#if 0
        printf("Merging %08x\n", merge_bit);
        print_product(stdout, p);
        print_product(stdout, p2);
#endif	

        c = p;
        while(merge_found == 0 && c != NULL) {
          if(c->next == p2) {
   	    // Remove p2 from the list and free it.
   	    c->next = c->next->next;
	    merge_found = 1;
	    free(p2);
            p->mask   &= ~merge_bit; 
            p->value  &= ~merge_bit;
#if 0
	    printf("=\n");
            print_product(stdout, p);
#endif	
	    return 1;
	  }
	  c = c->next;
	}
        fprintf(stderr,"ISSUE: Unable to find product to merge - ignoring\n");
      }
    }
    p2 = p2->next;
  }
  return 0;
}

/****************************************************************************/
int flag_merge_inner(struct Flag *flag) {
  int bit ;
  for(bit = 0; bit < 32; bit++) {
    int merged = 1;
    struct Product *p;
    uint32_t b = 1<<bit;

    while(merged == 1) {
      merged = 0;
      p = flag->first_product;
      while(merged == 0 && p != NULL) {
	merged = attempt_merge_inner(p,b);
	p = p->next;
      }
    }
  }
  return 1;
}
/****************************************************************************/
void join_pattern(uint32_t a_mask, uint32_t a_value, uint32_t b_mask, uint32_t b_value,
		  uint32_t *r_mask, uint32_t *r_value) {
  uint32_t common_mask;

  /* Just copy it over for now */
  *r_mask  = a_mask;
  *r_value = a_value;

  common_mask = a_mask & b_mask;
  // If any of the known bits do not match, then nothing matches.
  if((a_value & common_mask) != (b_value & common_mask)) {
    *r_mask  = 0;
    *r_value = 0;
  } else {
    // Merge the two masks togeather 
    *r_mask = a_mask | b_mask;
    *r_value = (a_value & a_mask) | (b_value & b_mask);
  }
  return;
}

/****************************************************************************/
void reduce_if_possible(struct Product *reducing, struct Product *type_list)
{
  uint32_t remove = 1<<31;
  while(remove != 0) {
    uint32_t mask  = reducing->mask;
    uint32_t value = reducing->value;
    if(reducing->mask & remove) {
      struct Product *p;
      mask  &= ~remove;
      value &= ~remove;
      p = type_list;
      while(p != NULL) {
	uint32_t a_mask, a_value;
	uint32_t b_mask, b_value;
        join_pattern(mask,          value,           p->mask, p->value, &a_mask, &a_value); 
        join_pattern(reducing->mask,reducing->value, p->mask, p->value, &b_mask, &b_value); 
	if(a_mask != b_mask || a_value != b_value) {
          break;
	}
	p = p->next;
      }
      if(p == NULL) {
        reducing->mask  &= ~remove;
	reducing->value &= reducing->mask;
      }
    }
    remove = remove >> 1;
  }
}

/****************************************************************************/
int process(void) {
  struct Pattern *p;
  struct Flag *flag;

  printf("Stage 1: Check for overlapping definitions\n");
  p = first_pattern;
  while(p->next != NULL) {
    struct Pattern *p2;
    p2 = p->next;
    while(p2 != NULL) {
      if(pattern_overlaps(p,p2)) {
        fprintf(stderr,"Overlap detected:\n");
	print_pattern(stderr,p);
	print_pattern(stderr,p2);
	return 0;
      }
      p2 = p2->next;
    }
    p = p->next;
  }

  printf("Stage 2: Build expressions for each output signal\n");
  p = first_pattern;
  while(p != NULL) {
    struct Flag_List *fl;
    fl = p->flag_list;
    while(fl != NULL) {
      struct Product *product;
      product = field_add_product(fl->flag, p->mask, p->value);
      if(product == NULL) {
        return 0;
      }
      fl = fl->next;
    }
    p = p->next;
  }
#if 0
  printf("Stage 3: Attach 'type' to all fields\n");
  p = first_pattern;
  while(p != NULL) {
    struct Flag_List *fl;
    struct Flag *type_flag = NULL;
    // Find the type for the pattern (one and only one alowed!)
    fl = p->flag_list;
    while(fl != NULL) {
      if(strncmp(fl->flag->name,"type_",5)==0) {
	if(type_flag != NULL) {
	  fprintf(stderr,"Multiple types for a pattern\n");
	  return 0;
	}
        type_flag = fl->flag;
      }
      fl = fl->next;
    }
    if(type_flag == NULL) {
      fprintf(stderr,"UNable to identify type for pattern\n");
      print_pattern(stderr, p);
      return 0;
    }
  
    // Add the type products to all the flags for this pattern 
    fl = p->flag_list;
    while(fl != NULL) {
      if(memcmp(fl->flag->name,"flag_",5)==0) {
	 struct Product *c;
	 c = type_flag->first_product;
	 while(c != NULL) {
           struct Product *product;
           product = field_add_type_product(fl->flag, c->mask, c->value);
           if(product == NULL) {
             return 0;
           }
           c = c->next;
	 }
      }
      fl = fl->next;
    }
    p = p->next;
  }

  printf("Stage 4: Perform an 'inner merge' on all flags\n");
  flag = first_flag;
  while(flag != NULL) {
    flag_merge_inner(flag);
    flag = flag->next;
  }

  printf("Stage 5: Remove redundant bits in fields\n");
  flag = first_flag;
  while(flag != NULL) {
    if(memcmp(flag->name,"flag_",5)==0) {
      struct Product *trying_to_reduce;
      trying_to_reduce = flag->first_product;
      while(trying_to_reduce != NULL) {
	 reduce_if_possible(trying_to_reduce, flag->first_type_product);
         trying_to_reduce = trying_to_reduce->next;  
      }
    }
    flag = flag->next;
  }

  printf("Stage 6: Attempt to merge again\n");
  flag = first_flag;
  while(flag != NULL) {
    flag_merge_inner(flag);
    flag = flag->next;
  }
#endif
  return 1;
}

/****************************************************************************/
int print_data(void) {
  struct Pattern *p;
  struct Flag *f;

  printf("-- Autogenerated RISC-V decoder\n");
  printf("--\n");
  printf("-- Definitions:\n");
  p = first_pattern;
  while(p != NULL) { 
    printf("--  ");
    print_pattern(stdout,p);
    p = p->next;
  }

  printf("--\n");
  printf("-- Resulting expressions:\n");
  f = first_flag;
  while(f != NULL) {
    if(f->first_product != NULL) {
      print_flag(stdout, f);
    }
    f = f->next;
  }

  return 1;
}

/****************************************************************************/
static int vhdl_emit_code_expression(FILE *file, uint32_t mask, uint32_t value) {
  int left,right;
  int first = 1;
  fprintf(file, "(");

  left =  31;
  while(left >= 0 && (mask & (1<<left)) == 0)
    left--;

  while(left >= 0) {
    int i;

    right = left;
    while(right > 0 && (mask & (1<<(right-1))) != 0) {
      right--;
    }
    
    if(!first)
      fprintf(file, " AND ");
    first = 0;
    fprintf(file, "(%s(%i DOWNTO %i) = \"", inst_name, left, right);
    for(i = left; i>=right; i--) 
       putc(value & (1<<i) ? '1' : '0', file);
    fprintf(file, "\")");

    left = right-1;
    while(left >= 0 && (mask & (1<<left)) == 0)
      left--;
  }
  fprintf(file, ")");
  return 1;
}
/****************************************************************************/
static int vhdl_emit_code(char *filename) {
   FILE *file;
   struct Flag *flag;
   struct Slice *s;
   file = fopen(filename,"w");
   if(file == NULL) {
     fprintf(stderr, "Unable to open file '%s'\n", filename);
     return 0;
   } 
  
   printf("Writing code to %s\n",filename); 
   fprintf(file, "LIBRARY ieee;\n");
   fprintf(file, "USE ieee.std_logic_1164.ALL;\n");
   fprintf(file, "\n");
   fprintf(file, "ENTITY %s IS\n", entity_name);
   fprintf(file, "    PORT (\n");
   fprintf(file, "        %-20s : in  std_logic_vector(31 downto 0);\n",inst_name);

   s = first_slice;
   while(s != NULL) {
	
     if(s->next == NULL && first_flag == NULL) {
       fprintf(file, "        %-20s : out std_logic_vector(%2i downto 0) := (others => '0'));\n", s->name, s->width-1);
     } else {
       fprintf(file, "        %-20s : out std_logic_vector(%2i downto 0) := (others => '0');\n", s->name, s->width-1);
     }
     s = s->next;
   }

   flag = first_flag;
   while(flag != NULL) {
      if(flag->next == NULL) {
         fprintf(file, "        %-20s : out std_logic := '0');\n", flag->name);    
      } else {
         fprintf(file, "        %-20s : out std_logic := '0';\n", flag->name);    
      }
      flag = flag->next;
   }
   fprintf(file, "END ENTITY;\n");
   fprintf(file, "\n");
   fprintf(file, "ARCHITECTURE %s OF %s IS\n", arch_name, entity_name);
   fprintf(file, "BEGIN\n");
   fprintf(file, "\n");

   // Output the slices
   s = first_slice;
   while(s != NULL) {
     int i;
     fprintf(file, "    %-20s <= ", s->name);
     for(i = s->width-1; i >= 0; i--) {
       if(s->indexes[i] != 0xFF) { 
	fprintf(file, "%s(%2i)", inst_name, s->indexes[i]);
       } else {
	fprintf(file, "'0'");
       }
       if(i != 0) {
	 fprintf(file, " & ");
       } else {
	 fprintf(file, ";\n");
       }
     } 
     s = s->next;
   }

   fprintf(file, "\n");

   // Now output all the flags
   fprintf(file, "flags_decode: PROCESS(%s)\n", inst_name);
   fprintf(file, "    BEGIN\n");

   flag = first_flag;
   while(flag != NULL) {
      struct Product *product;
      // Default output to false 
      fprintf(file, "        %s <= '0';\n", flag->name);    
      product = flag->first_product;

      while(product != NULL) {
	fprintf(file, "        IF ");
	vhdl_emit_code_expression(file, product->mask, product->value);
	fprintf(file, " THEN\n");
        fprintf(file, "            %s <= '1';\n", flag->name);    
	fprintf(file, "        END IF;\n");
	product = product->next;
      }
      fprintf(file, "\n");
      flag = flag->next;
   }


   fprintf(file, "    END PROCESS;\n");
   fprintf(file, "END %s;\n", arch_name);

   fclose(file);
   return 1;
}

/****************************************************************************/
int main(int argc, char *argv[]) {
  FILE *f;
  int lineno;
  char *filename;

  if(argc != 2) {
    fprintf(stderr,"No Input file given\n");
    return 1;
  }
  f = fopen(argv[1],"r");
  if(f == NULL) {
    fprintf(stderr,"Uable to open input file\n");
    return 1;
  }

  lineno = 0;
  while(read_line(f)) {
    if(!parse_line()) {
      fprintf(stderr,"Error parsing line %i\n", lineno);
      fprintf(stderr,"'%s'\n\n", buffer);
      return 1;
    }
    lineno++;
  }

  fprintf(stderr,"%i lines read in\n",lineno);

  if(!process()) {
    return 1;
  }

  print_data();

  filename = malloc(strlen(entity_name)+5);
  if(filename == NULL) {
     fprintf(stderr,"Out ot memory\n");
     return 0;
  }
  strcpy(filename,entity_name);
  strcat(filename,".vhd");
  vhdl_emit_code(filename);
  free(filename);

  fclose(f);
  return 0;
}
