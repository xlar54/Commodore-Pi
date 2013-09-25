int abs(int x)
{
	int t = x >> 31; 
	// t is -1 if x is negative otherwise it is 0
	return t ^ (x + t);
}
