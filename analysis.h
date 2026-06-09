class palette_class
{
  static int size;
public:
  static char** table;
  char * operator[](int i){return table[i%size];}
  palette_class();
};

void display_stub(iarray density, iarray species, iarray nsp, int ncells=1, 
		  int cell=0);

void connect_stub(iarray density, sparse_mat interaction, iarray nsp, 
		  int ncells=1);
