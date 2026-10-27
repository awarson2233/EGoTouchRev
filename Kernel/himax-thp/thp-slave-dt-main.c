// SPDX-License-Identifier: GPL-2.0
/*
 * 运行时给 gaokun3 打上触摸 Slave 的 DT overlay（gaokun3-thp-slave.dtso），不用改启动用的 dtb：
 * insmod 时应用，spi20 与其下的 touchscreen@0 随之出现并由 himax-thp 绑定；rmmod 时撤销。
 */

#include <linux/module.h>
#include <linux/of.h>

extern u8 __dtbo_gaokun3_thp_slave_begin[];
extern u8 __dtbo_gaokun3_thp_slave_end[];

static int thp_slave_ovcs_id;

static int __init thp_slave_dt_init(void)
{
	int ret;

	if (!of_machine_is_compatible("huawei,gaokun3")) {
		pr_err("thp-slave-dt: not a gaokun3, refusing to apply\n");
		return -ENODEV;
	}

	ret = of_overlay_fdt_apply(__dtbo_gaokun3_thp_slave_begin,
				   __dtbo_gaokun3_thp_slave_end - __dtbo_gaokun3_thp_slave_begin,
				   &thp_slave_ovcs_id, NULL);
	if (ret) {
		pr_err("thp-slave-dt: apply overlay failed: %d\n", ret);
		/* 应用到一半失败时 ovcs_id 仍然有效，要撤掉已经改了的部分 */
		if (thp_slave_ovcs_id)
			of_overlay_remove(&thp_slave_ovcs_id);
		return ret;
	}
	pr_info("thp-slave-dt: spi20 enabled for the touch slave\n");
	return 0;
}

static void __exit thp_slave_dt_exit(void)
{
	int ret = of_overlay_remove(&thp_slave_ovcs_id);

	if (ret)
		pr_err("thp-slave-dt: remove overlay failed: %d\n", ret);
}

module_init(thp_slave_dt_init);
module_exit(thp_slave_dt_exit);

MODULE_DESCRIPTION("Runtime DT overlay describing the gaokun3 touch slave on spi20");
MODULE_LICENSE("GPL");
