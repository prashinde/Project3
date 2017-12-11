#ifndef __DB_H_
#define __DB_H_
#include <mutex>
#include <unordered_map>
#include "transaction.h"
typedef struct database {
	unordered_map<unsigned long, double> map;
	mutex d_mutex;
} db_t;

int transaction(trans_t tr);
int create_record(unsigned long acc_nr, double amount);
int update_record(unsigned long acc_nr, double amount);
int search_record(unsigned long acc_nr, double *amount);
void db_sync();
#endif
