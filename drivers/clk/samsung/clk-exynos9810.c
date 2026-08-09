// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026
 *
 * Common Clock Framework support for the Exynos9810 SoC.
 */

#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include <dt-bindings/clock/samsung,exynos9810.h>

#include "clk.h"
#include "clk-exynos-arm64.h"

#define CLKS_NR_PERIC0	(CLK_GOUT_PERIC0_SYSREG_QCH + 1)

/* CMU_PERIC0 (0x10400000) */
#define PLL_CON0_MUX_CLKCMU_PERIC0_BUS_USER		0x0100
#define PLL_CON0_MUX_CLKCMU_PERIC0_IP_USER		0x0120
#define CLK_CON_GAT_GATE_CLK_PERIC0_USI03_USI		0x2018
#define CLK_CON_GAT_GOUT_USI03_USI_RSTNSYNC		0x206c
#define CLK_CON_GAT_GOUT_SYSREG_PERIC0_PCLK		0x2098
#define CLK_CON_GAT_GOUT_USI03_USI_IPCLK			0x20dc
#define CLK_CON_GAT_GOUT_USI03_USI_PCLK			0x20e0
#define QCH_CON_SYSREG_PERIC0_QCH			0x3014
#define QCH_CON_USI03_USI_QCH				0x3038

static const unsigned long peric0_clk_regs[] __initconst = {
	PLL_CON0_MUX_CLKCMU_PERIC0_BUS_USER,
	PLL_CON0_MUX_CLKCMU_PERIC0_IP_USER,
	CLK_CON_GAT_GATE_CLK_PERIC0_USI03_USI,
	CLK_CON_GAT_GOUT_USI03_USI_RSTNSYNC,
	CLK_CON_GAT_GOUT_SYSREG_PERIC0_PCLK,
	CLK_CON_GAT_GOUT_USI03_USI_IPCLK,
	CLK_CON_GAT_GOUT_USI03_USI_PCLK,
	QCH_CON_SYSREG_PERIC0_QCH,
	QCH_CON_USI03_USI_QCH,
};

PNAME(mout_peric0_bus_user_p) = { "oscclk", "peric0_bus" };
PNAME(mout_peric0_ip_user_p) = { "oscclk", "peric0_ip" };

static const struct samsung_mux_clock peric0_mux_clks[] __initconst = {
	nMUX(CLK_MOUT_PERIC0_BUS_USER, "mout_peric0_bus_user",
	     mout_peric0_bus_user_p, PLL_CON0_MUX_CLKCMU_PERIC0_BUS_USER,
	     4, 1),
	nMUX(CLK_MOUT_PERIC0_IP_USER, "mout_peric0_ip_user",
	     mout_peric0_ip_user_p, PLL_CON0_MUX_CLKCMU_PERIC0_IP_USER,
	     4, 1),
};

static const struct samsung_gate_clock peric0_gate_clks[] __initconst = {
	GATE(CLK_GOUT_PERIC0_SYSREG_QCH, "gout_peric0_sysreg_qch",
	     "mout_peric0_bus_user", QCH_CON_SYSREG_PERIC0_QCH, 0, 0, 0),
	GATE(CLK_GATE_PERIC0_USI03, "gate_peric0_usi03",
	     "mout_peric0_ip_user",
	     CLK_CON_GAT_GATE_CLK_PERIC0_USI03_USI, 21, 0, 0),
	GATE(CLK_GOUT_PERIC0_USI03_RSTNSYNC,
	     "gout_peric0_usi03_rstnsync", "gate_peric0_usi03",
	     CLK_CON_GAT_GOUT_USI03_USI_RSTNSYNC, 21, 0, 0),
	GATE(CLK_GOUT_PERIC0_USI03_QCH, "gout_peric0_usi03_qch",
	     "gout_peric0_usi03_rstnsync", QCH_CON_USI03_USI_QCH,
	     0, 0, 0),
	GATE(CLK_GOUT_PERIC0_USI03_IPCLK, "gout_peric0_usi03_ipclk",
	     "gout_peric0_usi03_qch",
	     CLK_CON_GAT_GOUT_USI03_USI_IPCLK,
	     21, 0, 0),
	GATE(CLK_GOUT_PERIC0_USI03_PCLK, "gout_peric0_usi03_pclk",
	     "gout_peric0_usi03_qch", CLK_CON_GAT_GOUT_USI03_USI_PCLK,
	     21, 0, 0),
	GATE(CLK_GOUT_PERIC0_SYSREG_PCLK, "gout_peric0_sysreg_pclk",
	     "gout_peric0_sysreg_qch",
	     CLK_CON_GAT_GOUT_SYSREG_PERIC0_PCLK, 21, CLK_IS_CRITICAL, 0),
};

static const struct samsung_cmu_info peric0_cmu_info __initconst = {
	.mux_clks		= peric0_mux_clks,
	.nr_mux_clks		= ARRAY_SIZE(peric0_mux_clks),
	.gate_clks		= peric0_gate_clks,
	.nr_gate_clks		= ARRAY_SIZE(peric0_gate_clks),
	.nr_clk_ids		= CLKS_NR_PERIC0,
	.clk_regs		= peric0_clk_regs,
	.nr_clk_regs		= ARRAY_SIZE(peric0_clk_regs),
	.clk_name		= "bus",
};

static int __init exynos9810_cmu_probe(struct platform_device *pdev)
{
	const struct samsung_cmu_info *info;
	struct device *dev = &pdev->dev;

	info = of_device_get_match_data(dev);
	exynos_arm64_register_cmu(dev, dev->of_node, info);

	return 0;
}

static const struct of_device_id exynos9810_cmu_of_match[] = {
	{
		.compatible = "samsung,exynos9810-cmu-peric0",
		.data = &peric0_cmu_info,
	},
	{ }
};

static struct platform_driver exynos9810_cmu_driver __refdata = {
	.driver = {
		.name = "exynos9810-cmu",
		.of_match_table = exynos9810_cmu_of_match,
		.suppress_bind_attrs = true,
	},
	.probe = exynos9810_cmu_probe,
};

static int __init exynos9810_cmu_init(void)
{
	return platform_driver_register(&exynos9810_cmu_driver);
}
core_initcall(exynos9810_cmu_init);
