#include<stdio.h> 
#include<stdlib.h> 
#include<math.h> 
#include<string.h> 
#include<conio.h> // ������ ����

long int p,q, t;	// ���ʷ� �Է� �޴� �� �Ҽ�, ���Ϸ� �����Լ�
long int m[100];	// �Է¹��� �޽���
long int n,e[100],d[100]; // �� �Ҽ��� ��, ����Ű, ����Ű
long int j,i, flag; 

int prime(long int);	// �Ҽ� ���� �Ǵ�
void ce();				// ����Ű ����
long int cd(long int);	// ����Ű ����

void main() 
{ 
	printf("\nENTER FIRST PRIME NUMBER\n"); 
	scanf("%d",&p); 
	flag=prime(p); 
	if(flag==0) 
	{ 
		printf("\nWRONG INPUT\n"); 
		getch(); 
		exit(1); 
	} 

	printf("\nENTER ANOTHER PRIME NUMBER\n"); 
	scanf("%d",&q); 
	flag=prime(q); 
	if(flag==0||p==q) 
	{ 
		printf("\nWRONG INPUT\n"); 
		getch(); 
		exit(1); 
	} 

	//�˰���
	n=p*q; 
	t=(p-1)*(q-1); 
	ce(); 

	// ����Ű, ����Ű ����
	printf("\n     Private-Key    ��     Public-Key    "); 
	printf("\n������������������������������������������");
	for(i=0;i<j-1;i++) 
		printf("\n[ %7.ld,%7.ld ] �� [ %7.ld,%7.ld ]\n",n, e[i], n, d[i]); 

	getch(); 
} 
int prime(long int pr) 
{ 
	int i; 
	j=sqrt(pr); 
	for(i = 2;i<=j;i++) 
	{ 
		if(pr%i==0) 
			return 0; 
	} 
	return 1; 
} 

// ���Ϸ� �����Լ��� ���μ��� ���迡 �ִ� e�� ���Ѵ�.
void ce() 
{ 
	int k; 
	k=0; 
	for(i=2;i<t;i++) 
	{
		
		// Gcd(e, ��(n)) = 1
		// Gcd�� ����� ����
		if(t%i==0) 
			continue; 
		flag=prime(i); 
		
		if(flag==1 && i != p && i != q) 
		{
			e[k] = i; // ����Ű
			flag = cd(e[k]); 
			if(flag > 0) 
			{ 
				d[k] = flag; // ����Ű
				k++; 
			} 
			if(k==99) 
				break; 
		} 
	} 
} 
long int cd(long int x) 
{ 
	long int k=1; 
	while(1) 
	{ 
		k=k+t; 
		if(k%x==0) 
			return(k/x); 
	} 
} 