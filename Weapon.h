#include <string>

class Weapon
{
public:
	Weapon();
	~Weapon();
	void Setup(std::string n, int d, int a);
	int GetDamage();
	std::string GetName();
	void Reload();
	bool CanShoot();
	void Shoot();
	void AddAmmo(float percentage);
	float GetLoadedAmmo();
private:
	std::string name = "Pistol";
	int damage = 20;
	int magazineSize = 0;

	int loadedAmmo = 0;
	int reserveAmmo;
	int maxAmmo;

};