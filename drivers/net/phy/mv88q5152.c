// SPDX-License-Identifier: GPL-2.0
/* PHY Marvell 88Q5152 device driver
 *
 * Copyright (C) 2023 Renesas Electronics Corporation
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/hwmon.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/marvell_phy.h>
#include <linux/bitfield.h>
#include <linux/of.h>

#include <linux/io.h>
#include <asm/irq.h>
#include <linux/uaccess.h>

#define PORT_STATUS		(0)

#define CMODE_SGMII		(0x0a)
#define CMODE_5GBASER		(0x0c)

#define PORT_LINK		BIT(11)
#define PORT_DUPLEX		BIT(10)
#define PORT_SPEED		GENMASK(9, 8)

enum port_speed_mode {
	PORT_SPEED_MODE_0 = 0,
	PORT_SPEED_MODE_1 = 0x100,
	PORT_SPEED_MODE_2 = 0x200,
	PORT_SPEED_MODE_3 = 0x300,
};

static int mv88q5152_probe(struct phy_device *phydev)
{
	/* Support Port 8, 9 only */
	switch (phydev->mdio.addr) {
	case 8:
	case 9:
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int mv88q5152_config_init(struct phy_device *phydev)
{
	switch (phydev->interface) {
	case PHY_INTERFACE_MODE_SGMII:
		phy_write(phydev, PORT_STATUS, CMODE_SGMII);
		break;
	case PHY_INTERFACE_MODE_5GBASER:
		phydev->autoneg = AUTONEG_DISABLE;
		phy_write(phydev, PORT_STATUS, CMODE_5GBASER);
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int mv88q5152_config_aneg(struct phy_device *phydev)
{
	return 0;
}

static int mv88q5152_read_status(struct phy_device *phydev)
{
	u16 val;

	phydev->link = 0;
	phydev->speed = SPEED_UNKNOWN;
	phydev->duplex = DUPLEX_UNKNOWN;

	val = phy_read(phydev, PORT_STATUS);

	/* FIXME: The port status just indicates the SerDes's information
	 * but not the internal PHY, so cannot detect the actual ones.
         */
	if (val & PORT_LINK) {
		phydev->link = 1;

		if (val & PORT_DUPLEX)
			phydev->duplex = DUPLEX_FULL;
		else
			phydev->duplex = DUPLEX_HALF;


		if  (phydev->interface == PHY_INTERFACE_MODE_SGMII) {
			switch (val & PORT_SPEED) {
			case PORT_SPEED_MODE_0:
				phydev->speed = SPEED_10;
				break;
			case PORT_SPEED_MODE_1:
				phydev->speed = SPEED_100;
				break;
			case PORT_SPEED_MODE_2:
				phydev->speed = SPEED_1000;
				break;
			}
		} else if (phydev->interface == PHY_INTERFACE_MODE_5GBASER) {
			phydev->speed = SPEED_5000;
		}
	}

	return 0;
}

static struct phy_driver mv88q5152_drivers[] = {
	{
		.phy_id = MARVELL_PHY_ID_88Q5152,
		.phy_id_mask = MARVELL_PHY_ID_MASK,
		.name = "Marvell 88Q5152",
		.probe = mv88q5152_probe,
		.config_init = mv88q5152_config_init,
		.config_aneg = mv88q5152_config_aneg,
		.read_status = mv88q5152_read_status,
	},
};

module_phy_driver(mv88q5152_drivers);

static struct mdio_device_id __maybe_unused mv88q5152_tbl[] = {
	{ MARVELL_PHY_ID_88Q5152, MARVELL_PHY_ID_MASK },
	{ },
};

MODULE_DEVICE_TABLE(mdio, mv88q5152_tbl);
