#ifndef __TRANS_H_
#define __TRANS_H_

enum commit_state {
	INIT = 1,
	READY,
	COMMIT,
	ABORT,
	COMMIT_STATE_NR,
};

enum commit_msg_type {
	PREPARE = COMMIT_STATE_NR+1,
	OK_M,
	ABORT_M,
	VOTE_COMMIT,
	VOTE_COMMIT_REP,
	COMMIT_M,
	COMMIT_MSG_TYPE_NR,
};

enum op_type {
	CREATE = COMMIT_MSG_TYPE_NR+1,
	UPDATE,
	QUERY,
	OP_TYPE_NR,
};

enum vote {
	COMMIT_V = OP_TYPE_NR+1,
	ABORT_V,
	VOTE_NR,
};

typedef struct transaction {
	enum op_type t_op_type;
	enum commit_state t_state;
	unsigned long trans_id;
	unsigned long acc_nr;
	double amount;
	int status;
} trans_t;

typedef struct two_pc_msg {
	enum commit_msg_type cmt_type;
	trans_t trans;
	enum vote cmt_vote;
} two_pc_msg_t;

trans_t *new_transaction();
void commit_transaction(trans_t *trans, coomdt_t *cmt);
#endif
