// SPDX-License-Identifier: GPL-2.0-only
/*
 * UFS PHY driver data for Samsung Exynos9810 SoC
 */

#include <linux/delay.h>

#include "phy-samsung-ufs.h"

#define EXYNOS9810_EMBEDDED_COMBO_PHY_CTRL		0x724
#define EXYNOS9810_EMBEDDED_COMBO_PHY_CTRL_MASK		BIT(0)
#define EXYNOS9810_EMBEDDED_COMBO_PHY_CTRL_EN		BIT(0)

#define EXYNOS9810_PHY_TRSV_CH_OFFSET			0x140
#define EXYNOS9810_PHY_CDR_AFC_STATUS			0x1fc
#define EXYNOS9810_PHY_CDR_AFC_CTRL			0x0f0

#define EXYNOS9810_PHY_COMN_REG_CFG(o, v, d) \
	PHY_COMN_REG_CFG((o) >> 2, (v), (d))
#define EXYNOS9810_PHY_TRSV_REG_CFG(o, v, d) \
	PHY_TRSV_REG_CFG_OFFSET((o) >> 2, (v), (d), \
				 EXYNOS9810_PHY_TRSV_CH_OFFSET >> 2)

static const struct samsung_ufs_phy_cfg exynos9810_pre_init_cfg[] = {
	EXYNOS9810_PHY_COMN_REG_CFG(0x08c, 0x80, PWR_MODE_ANY),
	EXYNOS9810_PHY_COMN_REG_CFG(0x074, 0x10, PWR_MODE_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x110, 0xb5, PWR_MODE_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x134, 0x43, PWR_MODE_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x16c, 0x20, PWR_MODE_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x178, 0xc0, PWR_MODE_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x0e0, 0x12, PWR_MODE_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x164, 0x58, PWR_MODE_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x1b0, 0x18, PWR_MODE_ANY),
	EXYNOS9810_PHY_COMN_REG_CFG(0x08c, 0xc0, PWR_MODE_ANY),
	EXYNOS9810_PHY_COMN_REG_CFG(0x08c, 0x00, PWR_MODE_ANY),
	END_UFS_PHY_CFG,
};

/* Exynos9810 boots its embedded UFS device in HS-G3 series B. */
static const struct samsung_ufs_phy_cfg exynos9810_pre_pwr_hs_cfg[] = {
	EXYNOS9810_PHY_TRSV_REG_CFG(0x0c8, 0xbc, PWR_MODE_HS_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x0f0, 0x7f, PWR_MODE_HS_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x120, 0xc0, PWR_MODE_HS_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x128, 0x00, PWR_MODE_HS_G3_SER_B),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x134, 0x63, PWR_MODE_HS_G3_SER_B),
	END_UFS_PHY_CFG,
};

static const struct samsung_ufs_phy_cfg exynos9810_post_pwr_hs_cfg[] = {
	END_UFS_PHY_CFG,
};

static const struct samsung_ufs_phy_cfg exynos9810_post_hibern8_enter[] = {
	EXYNOS9810_PHY_TRSV_REG_CFG(0x0c4, 0x99, PWR_MODE_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x0e8, 0x7f, PWR_MODE_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x0f0, 0x7f, PWR_MODE_ANY),
	EXYNOS9810_PHY_COMN_REG_CFG(0x004, 0x02, PWR_MODE_ANY),
	END_UFS_PHY_CFG,
};

static const struct samsung_ufs_phy_cfg exynos9810_pre_hibern8_exit[] = {
	EXYNOS9810_PHY_COMN_REG_CFG(0x004, 0x3f, PWR_MODE_HS_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x0c4, 0xd9, PWR_MODE_ANY),
	EXYNOS9810_PHY_TRSV_REG_CFG(0x0e8, 0x77, PWR_MODE_ANY),
	END_UFS_PHY_CFG,
};

static const struct samsung_ufs_phy_cfg
		*exynos9810_ufs_phy_cfgs[CFG_TAG_MAX] = {
	[CFG_PRE_INIT] = exynos9810_pre_init_cfg,
	[CFG_PRE_PWR_HS] = exynos9810_pre_pwr_hs_cfg,
	[CFG_POST_PWR_HS] = exynos9810_post_pwr_hs_cfg,
};

static const struct samsung_ufs_phy_cfg *exynos9810_hibern8_cfgs[] = {
	[CFG_POST_HIBERN8_ENTER] = exynos9810_post_hibern8_enter,
	[CFG_PRE_HIBERN8_EXIT] = exynos9810_pre_hibern8_exit,
};

static const char * const exynos9810_ufs_phy_clks[] = {
	"ref_clk",
};

static int exynos9810_ufs_phy_wait_for_cal(struct phy *phy, u8 lane)
{
	if (!lane)
		usleep_range(200, 210);

	return 0;
}

static int exynos9810_ufs_phy_wait_for_cdr(struct phy *phy, u8 lane)
{
	struct samsung_ufs_phy *ufs_phy = get_samsung_ufs_phy(phy);
	void __iomem *status = ufs_phy->reg_pma +
		EXYNOS9810_PHY_CDR_AFC_STATUS +
		lane * EXYNOS9810_PHY_TRSV_CH_OFFSET;
	void __iomem *ctrl = ufs_phy->reg_pma + EXYNOS9810_PHY_CDR_AFC_CTRL +
		lane * EXYNOS9810_PHY_TRSV_CH_OFFSET;
	u32 mask, val;
	int i;

	if (ufs_phy->ufs_phy_state == CFG_POST_PWR_HS) {
		mask = BIT(6);
	} else {
		mask = BIT(0);
		usleep_range(10, 20);
		writel(0xff, ctrl);
	}

	for (i = 0; i < 100; i++) {
		usleep_range(40, 50);
		val = readl(status);
		if ((val & mask) == mask)
			return 0;

		usleep_range(1, 2);
		writel(0x7f, ctrl);
		writel(0xff, ctrl);
	}

	dev_err(ufs_phy->dev, "failed to get PHY CDR AFC lock on lane %u\n",
		lane);
	return -ETIMEDOUT;
}

const struct samsung_ufs_phy_drvdata exynos9810_ufs_phy = {
	.cfgs = exynos9810_ufs_phy_cfgs,
	.cfgs_hibern8 = exynos9810_hibern8_cfgs,
	.isol = {
		.offset = EXYNOS9810_EMBEDDED_COMBO_PHY_CTRL,
		.mask = EXYNOS9810_EMBEDDED_COMBO_PHY_CTRL_MASK,
		.en = EXYNOS9810_EMBEDDED_COMBO_PHY_CTRL_EN,
	},
	.clk_list = exynos9810_ufs_phy_clks,
	.num_clks = ARRAY_SIZE(exynos9810_ufs_phy_clks),
	.wait_for_cal = exynos9810_ufs_phy_wait_for_cal,
	.wait_for_cdr = exynos9810_ufs_phy_wait_for_cdr,
};
