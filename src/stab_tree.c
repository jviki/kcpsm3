/**
 * stab_tree.c
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

#include "stab.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct stab {
	struct stab *less;
	struct stab *greater;
	struct stab_data *data;
};

struct stab *stab_init(void)
{
	return (struct stab *) calloc(1, sizeof(struct stab));
}

void stab_destroy(struct stab *stab)
{
	// TODO: make non recursive?
	if(stab != NULL) {
		stab_destroy(stab->less);
		stab_destroy(stab->greater);

		if(stab->data != NULL)
			stab_freedata(stab->data);

		free(stab);
	}
}

struct stab_data *stab_find(struct stab *stab, char *key)
{
	struct stab *curr = stab;
	const size_t keylen = strlen(key);

	while(curr != NULL) {
		size_t data_keylen;
		const char *data_key = stab_datakey(curr->data, &data_keylen);
		const int cmp = stab_cmpkey(key, keylen, data_key, data_keylen);//strcmp(key, datakey);

		if(!cmp)
			return curr->data;
		else if(cmp > 0)
			curr = curr->greater;
		else /*if (cmp < 0)*/ 
			curr = curr->less;
	}

	// was not found
	return NULL;
}

struct stab_data *stab_insert(struct stab *stab, char *key)
{
	assert(stab != NULL);
	assert(key != NULL);

	if(stab->data == NULL) {
		assert(stab->less == NULL);
		assert(stab->greater == NULL);

		stab->data = stab_newdata(key);
		return stab->data;
	}

	struct stab **target;	// pointer to pointer to the current node	
	struct stab *curr = stab; // current node
	const size_t keylen = strlen(key);

	while(curr != NULL) {
		size_t data_keylen;
		const char *data_key = stab_datakey(curr->data, &data_keylen);
		const int cmp = stab_cmpkey(key, keylen, data_key, data_keylen);

		if(!cmp) { // found
			return curr->data;
		}
		else if(cmp > 0) { // go right
			target = &curr->greater;
			curr = curr->greater;
		}
		else /*if (cmp < 0)*/ { // go left
			target = &curr->less;
			curr = curr->less;
		}
	}

	// not found, create new node
	*target = stab_init();
	if(*target == NULL)
		return NULL;

	struct stab_data *data = stab_newdata(key);
	if(data == NULL) {
		free(*target);
		return NULL;
	}

	(*target)->data = data;
	return data;
}

void stab_visit(struct stab *stab, visitor_f visit, void *op)
{
	if(stab != NULL) {
		visit(stab->data, op);
		stab_visit(stab->less, visit, op);
		stab_visit(stab->greater, visit, op);
	}
}

