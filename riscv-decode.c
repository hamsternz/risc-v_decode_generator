#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

static char entity_name[] = "decoder";
static char arch_name[] = "auto_generated_code";
static char inst_name[] = "inst";
char buffer[1024];
int mode;
#define MODE_FIELDS 1
#define MODE_FLAGS 2

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

  // Add to the list
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

    if(f->name[i] == '\0')
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

  if(strncmp(ptr,"Fields:",7) == 0) {
    mode = MODE_FIELDS;
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
    case MODE_FIELDS:
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
int process(void) {
  struct Pattern *p;
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

  printf("Stage 3: Perform an 'inner merge' on all flags\n");
  struct Flag *flag = first_flag;
  while(flag != NULL) {
    flag_merge_inner(flag);
    flag = flag->next;
  }

  printf("Stage 4: Attach 'type' to all fields\n");
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

  printf("Stage 5: identify and remove common bits in the 'type' products\n");
  flag = first_flag;
  while(flag != NULL) {
    struct Product *c;
    uint32_t common_mask = 0xFFFFFFFF;

    if(flag->first_type_product != NULL) {
      // Find out which mask bits are present in all type products
      c = flag->first_type_product;
    
      while(c != NULL) {
        common_mask &= c->mask;
        c = c->next;
      }

      // See which are common for all type products
      if(flag->first_type_product != NULL && flag->first_type_product->next != NULL) {
        c = flag->first_type_product->next;
        while(c != NULL) {
	  uint32_t same;
  	  same = ~(flag->first_type_product->value ^ c->value);
          same &= common_mask;
  	  common_mask = same;
          c = c->next;
        }
      }

      if(common_mask != 0) {
        // Now clear the common bits in the products
        c = flag->first_product;
        while(c != NULL) {
          c->mask &= ~common_mask;
          c = c->next;
        }
      }
    }
    flag = flag->next;
  }

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
static int vhdl_emit_code_expression(uint32_t mask, uint32_t value) {
  int left,right;
  int first = 1;
  printf("(");

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
      printf(" AND ");
    first = 0;
    printf("(%s(%i DOWNTO %i) = \"", inst_name, left, right);
    for(i = left; i>=right; i--) 
       putchar(value & (1<<i) ? '1' : '0');
    printf("\")");

    left = right-1;
    while(left >= 0 && (mask & (1<<left)) == 0)
      left--;
  }
  printf(")");
  return 1;
}
/****************************************************************************/
static int vhdl_emit_code(void) {
   struct Flag *flag;
   
   printf("LIBRARY ieee;\n");
   printf("USE ieee.std_logic_1164.ALL;\n");
   printf("\n");
   printf("ENTITY %s IS\n", entity_name);
   printf("    PORT (\n");
   printf("        %-20s : in  std_logic_vector(31 downto 0);\n",inst_name);
   flag = first_flag;
   while(flag != NULL) {
      if(flag->next == NULL) {
         printf("        %-20s : out std_logic := '0');\n", flag->name);    
      } else {
         printf("        %-20s : out std_logic := '0';\n", flag->name);    
      }
      flag = flag->next;
   }
   printf("END ENTITY;\n");
   printf("\n");
   printf("ARCHITECTURE %s OF %s IS\n", arch_name, entity_name);
   printf("BEGIN\n");
   printf("\n");
   printf("flags_decode: PROCESS(%s)\n", inst_name);
   printf("    BEGIN\n");

   flag = first_flag;
   while(flag != NULL) {
      struct Product *product;
      // Default output to false 
      printf("        %s <= '0';\n", flag->name);    
      product = flag->first_product;

      while(product != NULL) {
	printf("        IF ");
	vhdl_emit_code_expression(product->mask, product->value);
	printf(" THEN\n");
        printf("            %s <= '1';\n", flag->name);    
	printf("        END IF;\n");
	product = product->next;
      }
      printf("\n");
      flag = flag->next;
   }


   printf("    END PROCESS;\n");
   printf("END %s;\n", arch_name);

   return 1;
}

/****************************************************************************/
int main(int argc, char *argv[]) {
  FILE *f;
  int lineno;
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
  vhdl_emit_code();
  fclose(f);
  return 0;
}
