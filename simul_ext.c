#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cabeceras.h"

#define LONGITUD_COMANDO 100
#define NUM_COMANDOS 8

void printBytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int comprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void leeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);
int buscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre);
void verDirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
void renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo);
int imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre);
void borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre,  FILE *fich);
int copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino,  FILE *fich);
void grabarDirectorio(EXT_ENTRADA_DIR *directorio, FILE *fich);
void grabarInodos(EXT_BLQ_INODOS *inodos, FILE *fich);
void grabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void grabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
void grabarDatos(EXT_DATOS *memdatos, FILE *fich);
char *leeLinea(int tam);
int palabraEnLista(char *palabra, char **lista, int tamLista);

char *listaComandos[NUM_COMANDOS] = {"bytemaps", "copy", "dir", "info", "imprimir", "rename", "remove", "salir"};

unsigned int primer_bloque_datos;

int main()
{
	char *comando = NULL;
	char *orden = (char *)malloc(LONGITUD_COMANDO);
	char *argumento1 = (char *)malloc(LONGITUD_COMANDO);
	char *argumento2 = (char *)malloc(LONGITUD_COMANDO);
	int i = 0, j = 0;
	unsigned long int m = 0;
   EXT_SIMPLE_SUPERBLOCK ext_superblock;
   EXT_BYTE_MAPS ext_bytemaps;
   EXT_BLQ_INODOS ext_blq_inodos;
   EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
   EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
   EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
   int entradadir = 0;
   int grabardatos = 0;
   FILE *fent = NULL;
     
   // Lectura del fichero completo de una sola vez
   fent = fopen("particion.bin","rb+");
   fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);    

   memcpy(&ext_superblock,(EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
   memcpy(&directorio,(EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
   memcpy(&ext_bytemaps,(EXT_BLQ_INODOS *)&datosfich[1], SIZE_BLOQUE);
   memcpy(&ext_blq_inodos,(EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
   memcpy(&memdatos,(EXT_DATOS *)&datosfich[4],MAX_BLOQUES_DATOS*SIZE_BLOQUE);
   
   primer_bloque_datos = ext_superblock.s_first_data_block;
   // Buce de tratamiento de comandos
   for (;;)
   {
		do 
      {
         printf (">> ");
         fflush(stdin);
         comando = leeLinea(LONGITUD_COMANDO);
         orden = strtok(comando, " ");
         argumento1 = strtok(NULL, " ");
         argumento2 = strtok(NULL, " ");   //Separa la orden y los argumentos
		} 
      while (comprobarComando(comando,orden,argumento1,argumento2) !=0);
	   if (strcmp(orden,"dir")==0) 
      {
         verDirectorio(directorio, &ext_blq_inodos);
      }
      else if (strcmp(orden,"bytemaps")==0) 
      {
         printBytemaps(&ext_bytemaps);
      }
      else if (strcmp(orden,"info")==0) 
      {
         leeSuperBloque(&ext_superblock);
      }
      else if(strcmp(orden, "remove")==0)
      {
         if(argumento1 == NULL)
         {
            printf("ERROR: El comando remove requiere un argumento.\n");
         }

         else
         {
            borrar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, argumento1, fent);
            grabardatos = 1;
         }
         
      }
      else if (strcmp(orden, "rename")==0)
      {

         if(argumento1 == NULL || argumento2 == NULL)
         {
            printf("ERROR: El comando rename requiere dos argumentos.\n");
         }

         else
         {
            renombrar(directorio, &ext_blq_inodos, argumento1, argumento2);
            grabardatos = 1;
         }

         
      }
      else if (strcmp(orden, "imprimir")==0)
      {
         if(argumento1 != NULL)
         {
            imprimir(directorio, &ext_blq_inodos, memdatos, argumento1);
         }

         else
         {
            printf("ERROR: El comando imprimir requiere un argumento.\n");
         }
         
      }

      else if (strcmp(orden, "copy")==0)
      {
         if(argumento1 == NULL || argumento2 == NULL)
         {
            printf("ERROR: El comando copy requiere dos argumentos.\n");
         }

         else
         {
            copiar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, memdatos, argumento1, argumento2, fent);
            grabardatos = 1;
         }
         
      }

      else if (strcmp(orden,"salir")==0)
      {
         fclose(fent);

         //Se graban los datos si se ha hecho un rename, copy o remove
         if (grabardatos)
         {
            fent = fopen("particion.bin","w+b");
            grabarSuperBloque(&ext_superblock, fent);
            grabarDirectorio(directorio, fent);
            grabarByteMaps(&ext_bytemaps, fent);
            grabarInodos(&ext_blq_inodos, fent);
            grabarDatos(memdatos, fent);
         }

         return 0;
      }
   }
}

//Para saber si una palabra esta en una lista
int palabraEnLista(char *palabra, char **lista, int tamLista)
{
   int res = 0;   //Si es 0, no está; si es 1, está
   for(int i = 0; i < tamLista; i++)
   {
      if(strcmp(palabra, lista[i]) == 0)
      {
         res = 1;
      }
   }
   return res;
}

char *leeLinea(int tam)
{
   int i = 0;
   char c = '\0';
   char *res = (char *)malloc(tam);
   do
   {
      c = getchar();
      if(c != '\n')
      {
         res[i++] = c;
      }
   }
   while(c != '\n' && i < tam);
   res[i] = '\0';
   return res;
}

int comprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2)
{
   int res = 1;  //Si es 0, es válido. Si no, es invalido
   if(palabraEnLista(orden, listaComandos, NUM_COMANDOS) == 1)
   {
      res = 0;
   }
   else
   {
      printf("ERROR: Comando ilegal [bytemaps, copy, dir, info, imprimir, rename, remove, salir]\n");
   }
   return res;
}

void printBytemaps(EXT_BYTE_MAPS *ext_bytemaps)
{
   printf("Bytemap de bloques [0-24]:\n");
   for(int i = 0; i < 25; i++)
   {
      printf("%d ", (*ext_bytemaps).bmap_bloques[i] );
   }
   printf("\n");
   printf("Bytemap de inodos:\n");
   for(int i = 0; i < sizeof((*ext_bytemaps).bmap_inodos); i++)
   {
      printf("%d ", (*ext_bytemaps).bmap_inodos[i] );
   }
   printf("\n");
}

void leeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup)
{
   printf("Bloque %d Bytes\n", (*psup).s_block_size);
   printf("inodos particion = %d\n", (*psup).s_inodes_count);
   printf("inodos libres = %d\n", (*psup).s_free_inodes_count);
   printf("Bloques particion = %d\n", (*psup).s_blocks_count);
   printf("Bloques libres = %d\n", (*psup).s_free_blocks_count);
   printf("Primer bloque de datos = %d\n", (*psup).s_first_data_block);
}

void verDirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos)
{
   printf("Ficheros :\n");   
   for (int i = 1; i < MAX_FICHEROS; i++) 
   {
      //Comprobamos que el inodo sea valido, es decir que no este vacio
      if (directorio[i].dir_inodo != NULL_INODO)  
      {
         //Accedemos al inodo de cada archivo, lugar en el que estan los metadatos del archivo (tamanio,nombre...)
         EXT_SIMPLE_INODE *inodo = &inodos->blq_inodos[directorio[i].dir_inodo];  
         //Imprimimos los datos 
         printf("Nombre: %s\t", directorio[i].dir_nfich);
         printf("Tamaño: %d\t", inodo->size_fichero);    
         printf("Inodo: %d\t", directorio[i].dir_inodo);  
         printf("Bloques: ");
         //Recorre los bloques
         for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) 
         {
            //Comprueba que el bloque no este vacio
            if (inodo->i_nbloque[j] != NULL_BLOQUE)
            {
               printf("%d ", inodo->i_nbloque[j]); 
            }
         }
         printf("\n");
      }
   }
}

int buscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre)  //Devuelve el índice del fichero en el directorio si lo encuentra, y -1 si no
{
   int encontrado = -1;
   //Recorre los nombres hasta que lo encuentre. Si no lo encuentra, se queda en -1
   for(int i = 0; (encontrado == -1) && (i < sizeof(directorio)); i++)
   {
      if(strcmp(directorio[i].dir_nfich, nombre) == 0)
      {
         encontrado = i;
      }
   }
   return encontrado;
}

void renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo)
{
   int posicionNombreAntiguo = -1, posicionNombreNuevo = -1;
   posicionNombreAntiguo = buscaFich(directorio, inodos, nombreantiguo);
   posicionNombreNuevo = buscaFich(directorio, inodos, nombrenuevo);
   //Comprobar que el fichero a renombrar existe
   if(posicionNombreAntiguo == -1)
   {
      printf("ERROR: Fichero %s no encontrado\n", nombreantiguo);
   }
   //Comprobar que no existe un fichero con el nombre nuevo a renombrar
   else if(posicionNombreNuevo != -1)
   {
      printf("ERROR: El fichero %s ya existe\n", nombrenuevo);
   }
   else
   {
      memcpy(directorio[posicionNombreAntiguo].dir_nfich, nombrenuevo, LEN_NFICH); //Cambio de nombre exitoso
      printf("El nombre de %s se ha cambiado a %s con éxito.\n", nombreantiguo, nombrenuevo);
   }
}

void borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich)
{
   int inodo_a_eliminar = -1, salir = 0;
   
   //Buscar si existe el fichero a eliminar
   for (int i = 0; i < MAX_FICHEROS && !salir; i++) 
   {
      if (strcmp(directorio[i].dir_nfich, nombre) == 0) 
      {
         inodo_a_eliminar = directorio[i].dir_inodo;
         salir = 1;
      }
   }

   //Si no se encuentra el fichero, devuelve un error
   if (inodo_a_eliminar == -1) 
   {
      printf("Error: fichero no encontrado\n");  
   }
   //Liberar el inodo correspondiente
   else
   {
      EXT_SIMPLE_INODE *inodo = &inodos->blq_inodos[inodo_a_eliminar];
      //Se marca el inodo como libre en el bytemap de inodos
      ext_bytemaps->bmap_inodos[inodo_a_eliminar] = 0;
      //Establecer el tamaño del fichero a 0
      inodo->size_fichero = 0;
      //Liberamos los bloques de datos
      for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) 
      {
         if (inodo->i_nbloque[i] != NULL_BLOQUE) 
         {
            ext_superblock->s_free_blocks_count++;  //Los bloques liberados ahora los pueden usar otros ficheros
            ext_bytemaps->bmap_bloques[inodo->i_nbloque[i]] = 0;
         }
      }
      //Marcar los punteros de bloques como libres 
      for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) 
      {
         inodo->i_nbloque[i] = NULL_BLOQUE;
      }
      ext_superblock->s_free_inodes_count++;  //El inodo queda libre también

      //Eliminar la entrada del directorio
      salir = 0;
      for (int i = 0; i < MAX_FICHEROS && !salir; i++) 
      {
         if (directorio[i].dir_inodo == inodo_a_eliminar) 
         {
         //Poner el nombre vacío y el número de inodo a NULL_INODO
         memset(directorio[i].dir_nfich, 0, LEN_NFICH);
         directorio[i].dir_inodo = NULL_INODO;
         salir = 1;
         }
      }  
      printf("Fichero borrado con exito\n");
   }
   
}

int imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre)
{

   int indice = buscaFich(directorio, inodos, nombre);   //Para ver si existe el fichero

   if(indice != -1)
   {

      EXT_SIMPLE_INODE inodo = inodos->blq_inodos[directorio[indice].dir_inodo];

      for(int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++)
      {

         if (inodo.i_nbloque[i] != NULL_BLOQUE)
         {
            //Como memdatos tiene los bloques de datos, hay que restar al índice (que es de todos los bloques) el indice del primer bloque de datos
            printf("%s", memdatos[inodo.i_nbloque[i] - primer_bloque_datos].dato);  //Imprime los contenidos de los bloques
         }
         
      }

      printf("\n");
   }

   else
   {
      printf("Fichero no encontrado: %s.\n", nombre);
   }

   return 0;
}

int copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino, FILE *fich)
{
   int salir = 0;
   int indice_origen = buscaFich(directorio, inodos, nombreorigen);
   if (indice_origen == -1) 
   {
      printf("ERROR: Fichero %s no encontrado.\n", nombreorigen);
      return -1; //Si el fichero origen no existe, sale y devuelve -1
   }


   if (buscaFich(directorio, inodos, nombredestino) >= 0) 
   {
      printf("ERROR: El fichero %s ya existe.\n", nombredestino);
      return -1;  //Si el fichero destino existe, sale y devuelve -1
   }

   
   int nuevo_inodo = -1;   //Para buscar un nuevo inodo

   //Por si no hay suficientes inodos
   if(ext_superblock->s_free_inodes_count == 0)
   {
      printf("ERROR: No hay inodos libres.\n");
      return -1; 
   }

   EXT_SIMPLE_INODE *inodo_origen = &inodos->blq_inodos[directorio[indice_origen].dir_inodo];

   int bloques_necesarios = (inodo_origen->size_fichero + SIZE_BLOQUE - 1)/SIZE_BLOQUE;

   if (ext_superblock->s_free_blocks_count < bloques_necesarios)   //Si no hay bloques suficientes
   {
      printf("ERROR: No hay bloques suficientes para copiar el fichero.\n");
      return -1;
   }

   for (int i = 0; i < MAX_INODOS && nuevo_inodo == -1; i++)   
   {
      if (ext_bytemaps->bmap_inodos[i] == 0) 
      {
         nuevo_inodo = i;
         ext_bytemaps->bmap_inodos[i] = 1;   //Se actualiza el bytemap
         ext_superblock->s_free_inodes_count--;
      }
   }


   //Creamos un nuevo inodo para el fichero destino y copiamos el inodo del fichero destino
   EXT_SIMPLE_INODE *inodo_destino = &inodos->blq_inodos[nuevo_inodo];
   memcpy(inodo_destino, inodo_origen, sizeof(EXT_SIMPLE_INODE)); 

   //Copiamos también los bloques
   
   for (int i = 0; i < bloques_necesarios; i++) 
   {
      for (int j = 0; j < MAX_BLOQUES_DATOS && salir == 0; j++) 
      {
         if (ext_bytemaps->bmap_bloques[j] == 0) 
         {
            ext_bytemaps->bmap_bloques[j] = 1; 
            ext_superblock->s_free_blocks_count--;
            inodo_destino->i_nbloque[i] = j;

            
            memcpy(memdatos[j-primer_bloque_datos].dato, memdatos[inodo_origen->i_nbloque[i]-primer_bloque_datos].dato, SIZE_BLOQUE);
            salir = 1;

            //Si el bloque está libre, se lo asignamos y copiamos la información
         }
      }

      salir = 0;
   }

   

   //Buscamos una entrada libre en el directorio para poner el nuevo inodo
   for (int i = 0; i < MAX_FICHEROS && salir == 0; i++) 
   {
      if (directorio[i].dir_inodo == NULL_INODO) 
      {
         directorio[i].dir_inodo = nuevo_inodo;
         strncpy(directorio[i].dir_nfich, nombredestino, LEN_NFICH);
         salir = 1;
      }
   }
   salir = 0;

   printf("Fichero %s copiado a %s correctamente.\n", nombreorigen, nombredestino);
   return 0;
}

//Funciones para grabar los datos en la particion una vez se termina de usar

void grabarDirectorio(EXT_ENTRADA_DIR *directorio, FILE *fich) 
{
   fseek(fich, SIZE_BLOQUE * 3, SEEK_SET); 
   fwrite(directorio, SIZE_BLOQUE, 1, fich);
}

void grabarInodos(EXT_BLQ_INODOS *inodos, FILE *fich) 
{
   fseek(fich, SIZE_BLOQUE * 2, SEEK_SET); 
   fwrite(inodos, SIZE_BLOQUE, 1, fich);
}

void grabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich) 
{
   fseek(fich, SIZE_BLOQUE * 1, SEEK_SET); 
   fwrite(ext_bytemaps, SIZE_BLOQUE, 1, fich);
}

void grabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich) 
{
   fwrite(ext_superblock, SIZE_BLOQUE, 1, fich);
}

void grabarDatos(EXT_DATOS *memdatos, FILE *fich) 
{
   fseek(fich, SIZE_BLOQUE * 4, SEEK_SET);
   fwrite(memdatos, SIZE_BLOQUE, MAX_BLOQUES_DATOS, fich);
}

