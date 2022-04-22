#include "Weapon.h"

Weapon::Weapon()
{

}

Weapon::~Weapon()
{

}

void Weapon::Setup(std::string n, int d, int a)
{
	name = n;
	damage = d;
	magazineSize = a;

	loadedAmmo = magazineSize;
	maxAmmo = magazineSize * 7;
	reserveAmmo = maxAmmo;
}

int Weapon::GetDamage()
{
	return damage;
}

std::string Weapon::GetName()
{
	return name;
}

void Weapon::Reload()
{
	if (reserveAmmo > 0)
	{
		int ammoToCompleteMag = magazineSize - loadedAmmo;

		if (ammoToCompleteMag <= reserveAmmo)
		{
			loadedAmmo = magazineSize;
			reserveAmmo -= ammoToCompleteMag;;
		}
		else
		{
			loadedAmmo += reserveAmmo;
			reserveAmmo = 0;
		}
	}
}

bool Weapon::CanShoot()
{
	if (loadedAmmo > 0)
		return true;
	else return false;
}

void Weapon::Shoot()
{
	loadedAmmo--;
}

void Weapon::AddAmmo(float percentage)
{
	int ammoToRestore = (maxAmmo * (percentage/100));
	reserveAmmo += ammoToRestore;
	if (reserveAmmo > maxAmmo)
		reserveAmmo = maxAmmo;
}

float Weapon::GetLoadedAmmo()
{
	return (float)loadedAmmo;
}