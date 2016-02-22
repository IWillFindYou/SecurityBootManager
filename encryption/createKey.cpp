#include<stdio.h> 
#include<stdlib.h> 
#include<math.h> 
#include<string.h> 

long int p,q, t;	// 최초로 입력 받는 두 소수, 오일러 파이함수
long int m[100];	// 입력받은 메시지
long int n,e[100],d[100]; // 두 소수의 곱, 개인키, 공개키
long int j,i, flag; 

int prime(long int);	// 소수 인지 판단
void ce();				// 개인키 생성
long int cd(long int);	// 공개키 생성

int main() 
{ 
	printf("\nENTER FIRST PRIME NUMBER\n"); 
	scanf("%d",&p); 
	flag=prime(p); 
	if(flag==0) 
	{ 
		printf("\nWRONG INPUT\n"); 
		exit(1); 
	} 

	printf("\nENTER ANOTHER PRIME NUMBER\n"); 
	scanf("%d",&q); 
	flag=prime(q); 
	if(flag==0||p==q) 
	{ 
		printf("\nWRONG INPUT\n"); 
		exit(1); 
	} 

	//알고리즘	
	n=p*q; 
	t=(p-1)*(q-1); 
	ce(); 

	// 개인키, 공개키 생성	
	printf("\n     Private-Key    │     Public-Key    "); 
	printf("\n────────────────────┼────────────────────");
	for(i=0;i<j-1;i++) 
		printf("\n[ %7.ld,%7.ld ] │ [ %7.ld,%7.ld ]",n, e[i], n, d[i]); 
	printf("\n");
	
	return 0;
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

// 오일러 파이함수와 서로소의 관계에 있는 e를 구한다.
void ce() 

{ 
	int k; 
	k=0; 
	for(i=2;i<t;i++) 
	{

		// Gcd(e, φ(n)) = 1 
		// // Gcd와 결과가 같음
		if(t%i==0) 		
			continue; 
		flag=prime(i); 

		if(flag==1 && i != p && i != q) 
		{
			e[k] = i; // 개인키
			flag = cd(e[k]); 
			if(flag > 0) 
			{ 
				d[k] = flag; // 공개키
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
