#include "db.h"
db_t gdta;

int create_record(unsigned long acc_nr, double amount)
{
	unordered_map<unsigned long, double>::const_iterator got = gdta.map.find(acc_nr);
	if(got != gdta.map.end())
		return -EEXIST;
	gdta.map.insert(pair<unsigned long, double>(acc_nr, amount));
	return 0;
}

int update_record(unsigned long acc_nr, double amount)
{
	unordered_map<unsigned long, double>::const_iterator got = gdta.map.find(acc_nr);
	if(got == gdta.map.end())
		return -ENOENT;
	gdta.map[acc_nr] = amount;
	return 0;
}

int search_record(unsigned long acc_nr, double *amount)
{
	unordered_map<unsigned long, double>::const_iterator got = gdta.map.find(acc_nr);
	if(got == gdta.map.end())
		return -ENOENT;
	*amount = got->second;
	return 0;
}

int transaction(trans_t tr)
{
	int ret = -EINVAL;
	switch(tr.t_op_type) {
		case CREATE:
		ret = create_record(tr.acc_nr, tr.amount);
		break;
		case UPDATE:
		ret = update_record(tr.acc_nr, tr.amount);
		break;
		case QUERY:
		ret = search_record(tr.acc_nr, &tr.amount);
		break;
	}
	return ret;
}
