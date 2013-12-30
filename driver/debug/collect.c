
#include <cdev/fifo_cdev.h>

/*
 * param_put: put param array to fifo_dev.
 *
 * @param src: the param array
 * @param cnt: the src array lenth
 * @param dst: store the src array
 *
 * @return
 */
struct fifo_cdev *put_param_into_cdev(struct param_cmds_info *src,
		struct fifo_cdev *dst)
{
	int i, j = 0, param_size = 0, total_step = 0;
	struct param_dev *param_info;
	struct fifo_cdev *temp = dst;
	printk("clw: LCD off and %s E\n", __func__);

	if ((!temp) || (!src)) {
		printk("clw: can't get struct\n");
		return 0;
	}
	if(down_interruptible(&(temp->sem))) {
		return 0;
	}

	temp->params_info = src;
	/* store cmds cnt */
	temp->mem[total_step++] = src->cnt;

	param_info = src->cmds;
	if (!param_info) {
		printk("clw: error to get param_info struct\n");
		return 0;
	}

	/* store params size */
	for (i = 0; i < src->cnt; i++) {
		temp->mem[total_step++] = param_info[i].size;	
		j += param_info[i].size;
	}

	/* store the total params size */
	temp->mem[total_step++] = j;

#ifdef FIFO_CDEV_DEBUG
	printk("clw: total_cmds_params = %d\n", j);
#endif

	if ((total_step + j) > GLOBALFIFO_SIZE) {
		printk("clw: error, param_info_size too large");
		return 0;
	}

	/* store the param */
	for (i = 0; i < src->cnt; i++) {
		param_size = param_info[i].size;
		for (j = 0; j < param_size; j++) {
			temp->mem[total_step++] = 
						param_info[i].param[j];
		}
	}
	/* 保存当前长度 */
	temp->current_len = total_step;
	up(&(temp->sem));

	printk("clw: LCD off and %s X\n", __func__);
	return temp;
}

/*
 * param_get: get param array from fifo_dev.
 *
 * @param src: the fifo_dev data
 * @param dst: store the src array
 *
 * @return
 */
struct param_dev *get_param_from_cdev(struct fifo_cdev *src,
		struct param_cmds_info *dst)
{
	int i, j = 0, total_step = src->current_len;
	int step = 0, param_size = 0;
	struct param_dev *temp = NULL;
	printk("clw: LCD on and %s E\n", __func__);

	if ((!dst) || (!src)) {
		printk("clw: can't get struct\n");
		return 0;
	}
	if(down_interruptible(&(src->sem))) {
		return 0;
	}

	temp = dst->cmds;
	/* restore the cmds cnt */
	dst->cnt = src->mem[step++];

	/* restore the params size */
	for (i = 0; i < dst->cnt; i++) {
		temp[i].size = src->mem[step++];
	}
	/* restore the param total size */
	param_size = src->mem[step++];
#ifdef FIFO_CDEV_DEBUG
	printk("clw: total_step=%d, param_size=%d, dst->cnt=%d\n",
		   	total_step, param_size, dst->cnt);
#endif
	if (total_step != param_size + dst->cnt + 2) {
		printk("clw: the mem data error!!!\n");	
		return 0;
	}

	/* restore the params */
	for (i = 0; i < dst->cnt; i++) {
		for (j = 0; j < temp[i].size; j++) {
			temp[i].param[j] = src->mem[step++];
		}
	}

	up(&(src->sem));
	printk("clw: LCD on and %s X\n", __func__);
	return temp;
}

