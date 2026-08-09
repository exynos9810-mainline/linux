// SPDX-License-Identifier: GPL-2.0-only
/*
 * Minimal Samsung S2MPS18 regulator support for ACPM-managed rails.
 *
 * The Exynos9810 firmware owns the physical SPEEDY bus and exposes PMIC
 * register access through ACPM channel 2. Only the two touchscreen rails
 * are described here; other regulators can be added with their consumers.
 */

#include <linux/firmware/samsung/exynos-acpm-protocol.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>

#define S2MPS18_ACPM_CHANNEL	2
#define S2MPS18_SPEEDY_CHANNEL	0
#define S2MPS18_PMIC_TYPE	1

#define S2MPS18_LDO35_CTRL	0x6c
#define S2MPS18_LDO43_CTRL	0x74
#define S2MPS18_LDO_VSEL_MASK	GENMASK(5, 0)
#define S2MPS18_ENABLE_MASK	GENMASK(7, 6)
#define S2MPS18_ENABLE_NORMAL	S2MPS18_ENABLE_MASK
#define S2MPS18_LDO_N_VOLTAGES	64

struct s2mps18_info {
	struct acpm_handle *acpm;
};

static int s2mps18_read(struct s2mps18_info *info, u8 reg, u8 *val)
{
	return info->acpm->ops->pmic.read_reg(info->acpm,
			S2MPS18_ACPM_CHANNEL, S2MPS18_PMIC_TYPE, reg,
			S2MPS18_SPEEDY_CHANNEL, val);
}

static int s2mps18_update(struct s2mps18_info *info, u8 reg, u8 val,
			  u8 mask)
{
	return info->acpm->ops->pmic.update_reg(info->acpm,
			S2MPS18_ACPM_CHANNEL, S2MPS18_PMIC_TYPE, reg,
			S2MPS18_SPEEDY_CHANNEL, val, mask);
}

static int s2mps18_regulator_is_enabled(struct regulator_dev *rdev)
{
	struct s2mps18_info *info = rdev_get_drvdata(rdev);
	u8 val;
	int ret;

	ret = s2mps18_read(info, rdev->desc->enable_reg, &val);
	if (ret)
		return ret;

	return !!(val & S2MPS18_ENABLE_MASK);
}

static int s2mps18_regulator_enable(struct regulator_dev *rdev)
{
	struct s2mps18_info *info = rdev_get_drvdata(rdev);

	return s2mps18_update(info, rdev->desc->enable_reg,
				S2MPS18_ENABLE_NORMAL, S2MPS18_ENABLE_MASK);
}

static int s2mps18_regulator_disable(struct regulator_dev *rdev)
{
	struct s2mps18_info *info = rdev_get_drvdata(rdev);

	return s2mps18_update(info, rdev->desc->enable_reg, 0,
				S2MPS18_ENABLE_MASK);
}

static int s2mps18_regulator_get_voltage_sel(struct regulator_dev *rdev)
{
	struct s2mps18_info *info = rdev_get_drvdata(rdev);
	u8 val;
	int ret;

	ret = s2mps18_read(info, rdev->desc->vsel_reg, &val);
	if (ret)
		return ret;

	return val & S2MPS18_LDO_VSEL_MASK;
}

static int s2mps18_regulator_set_voltage_sel(struct regulator_dev *rdev,
					      unsigned int sel)
{
	struct s2mps18_info *info = rdev_get_drvdata(rdev);

	return s2mps18_update(info, rdev->desc->vsel_reg, sel,
				S2MPS18_LDO_VSEL_MASK);
}

static const struct regulator_ops s2mps18_regulator_ops = {
	.enable = s2mps18_regulator_enable,
	.disable = s2mps18_regulator_disable,
	.is_enabled = s2mps18_regulator_is_enabled,
	.list_voltage = regulator_list_voltage_linear,
	.map_voltage = regulator_map_voltage_linear,
	.get_voltage_sel = s2mps18_regulator_get_voltage_sel,
	.set_voltage_sel = s2mps18_regulator_set_voltage_sel,
};

#define S2MPS18_LDO_DESC(_name, _match, _id, _reg, _min) { \
	.name = _name, \
	.of_match = _match, \
	.regulators_node = "regulators", \
	.id = _id, \
	.ops = &s2mps18_regulator_ops, \
	.type = REGULATOR_VOLTAGE, \
	.owner = THIS_MODULE, \
	.min_uV = _min, \
	.uV_step = 25000, \
	.n_voltages = S2MPS18_LDO_N_VOLTAGES, \
	.vsel_reg = _reg, \
	.vsel_mask = S2MPS18_LDO_VSEL_MASK, \
	.enable_reg = _reg, \
	.enable_mask = S2MPS18_ENABLE_MASK, \
	.enable_time = 128, \
}

static const struct regulator_desc s2mps18_regulators[] = {
	S2MPS18_LDO_DESC("LDO35", "ldo35", 0, S2MPS18_LDO35_CTRL, 700000),
	S2MPS18_LDO_DESC("LDO43", "ldo43", 1, S2MPS18_LDO43_CTRL, 1800000),
};

static int s2mps18_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *regulators;
	struct s2mps18_info *info;
	u8 ldo35, ldo43;
	int i, ret;

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->acpm = devm_acpm_get_by_node(dev, dev->parent->of_node);
	if (IS_ERR(info->acpm))
		return dev_err_probe(dev, PTR_ERR(info->acpm),
				     "failed to get ACPM handle\n");

	regulators = of_get_child_by_name(dev->of_node, "regulators");
	if (!regulators)
		return dev_err_probe(dev, -ENODEV, "missing regulators node\n");

	for (i = 0; i < ARRAY_SIZE(s2mps18_regulators); i++) {
		const struct regulator_desc *desc = &s2mps18_regulators[i];
		struct regulator_config config = { .dev = dev, .driver_data = info };
		struct regulator_dev *rdev;

		config.of_node = of_get_child_by_name(regulators, desc->of_match);
		if (!config.of_node)
			continue;

		rdev = devm_regulator_register(dev, desc, &config);
		of_node_put(config.of_node);
		if (IS_ERR(rdev)) {
			ret = PTR_ERR(rdev);
			of_node_put(regulators);
			return dev_err_probe(dev, ret, "failed to register %s\n",
					     desc->name);
		}
	}
	of_node_put(regulators);

	ret = s2mps18_read(info, S2MPS18_LDO35_CTRL, &ldo35);
	if (ret)
		return dev_err_probe(dev, ret, "failed to read LDO35\n");
	ret = s2mps18_read(info, S2MPS18_LDO43_CTRL, &ldo43);
	if (ret)
		return dev_err_probe(dev, ret, "failed to read LDO43\n");

	dev_info(dev, "touchscreen rails: LDO35=%#02x LDO43=%#02x\n",
		 ldo35, ldo43);
	return 0;
}

static const struct of_device_id s2mps18_of_match[] = {
	{ .compatible = "samsung,s2mps18-pmic" },
	{ }
};
MODULE_DEVICE_TABLE(of, s2mps18_of_match);

static struct platform_driver s2mps18_driver = {
	.probe = s2mps18_probe,
	.driver = {
		.name = "s2mps18-regulator",
		.of_match_table = s2mps18_of_match,
	},
};
module_platform_driver(s2mps18_driver);

MODULE_AUTHOR("OpenAI");
MODULE_DESCRIPTION("Samsung S2MPS18 ACPM regulator driver");
MODULE_LICENSE("GPL");
