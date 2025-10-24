#include "fs.h"

/*inizializzazione globale (condivisa) di tutti i campi della struct FS a 0, tranne 
fd che inizializzo a -1 per indicare che non c'è nessun file aperto*/
FS fs={.fd=-1};