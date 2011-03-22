/**
 * stab.h
 * Author: Jan Viktorin <xvikto03 (at) stud.fit.vutbr.cz>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef _STAB_H
#define _STAB_H

#include <stdlib.h>

/**
 * Symbol table. 
 */
struct stab;

/**
 * Structure that holds information to be stored in the symbol table.
 */
struct stab_data;

/**
 * Allocates and initializes a stab_data instance.
 * It is to be implemented externaly for the concrete application.
 *
 * @param key to be stored in the data
 */
struct stab_data *stab_newdata(char *key);

/**
 * Destroys the stab_data instance.
 * It is to be implemented externaly for the concrete application.
 */
void stab_freedata(struct stab_data *data);

/**
 * Getter for key of the data.
 * It is to be implemented externaly for the concrete application.
 */
char *stab_datakey(struct stab_data *data, size_t *keylen);

/**
 * Compares two keys of the data.
 * It is to be implemented externaly for the concrete application.
 */
int stab_cmpkey(const char *a, const size_t alen, 
						const char *b, const size_t blen);

/**
 * Initializes the stab.
 * @return valid instance of stab or NULL on error
 */
struct stab *stab_init(void);

/**
 * Destroys the symbol table and all stab_data associated.
 */ 
void stab_destroy(struct stab *stab);

/**
 * Looks up for the given key in the symbol table.
 * 
 * @param stab table to be searched
 * @param key key to be looked up
 * @return assiciated stab_data if exists otherwise NULL
 */
struct stab_data *stab_find(struct stab *stab, char *key);

/**
 * Inserts the key in the symbol table.
 
 * @param stab table to be inserted into
 * @param key key to be inserted into the table 
 * @return associated stab_data with the if the key exists or
 *			new allocated stab_data if it does not exist or NULL on error
 */
struct stab_data *stab_insert(struct stab *stab, char *key);

/**
 * Visitor function.
 */
typedef void (*visitor_f)(struct stab_data *, void *);

/**
 * Visits all nodes in any order.
 */
void stab_visit(struct stab *stab, visitor_f visit, void *op);

#endif

