

// 注意：x的范围是1~MAPWIDTH，y的范围是1~MAPHEIGHT
// 数组是先行（y）后列（c）
// 坐标系：原点在左下角
 
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include "jsoncpp/json.h"
using namespace std;
 
#define MAPWIDTH 10
#define MAPHEIGHT 20
 
// 我所在队伍的颜色（0为红，1为蓝，仅表示队伍，不分先后）
int currBotColor;
int enemyColor;
 
// 先y后x，记录地图状态，0为空，1为以前放置，2为刚刚放置，负数为越界
// （2用于在清行后将最后一步撤销再送给对方）
int gridInfo[2][MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };
 
// 代表分别向对方转移的行
int trans[2][4][MAPWIDTH + 2] = { 0 };
 
// 转移行数
int transCount[2] = { 0 };
 
// 运行eliminate后的当前高度
int maxHeight[2] = { 0 };
 
// 总消去行数的分数之和
int elimTotal[2] = { 0 };
 
// 一次性消去行数对应分数
const int elimBonus[] = { 0, 1, 3, 5, 7 };
 
// 给对应玩家的各类块的数目总计
int typeCountForColor[2][7] = { 0 };
 
const int blockShape[7][4][8] = {
	{ { 0,0,1,0,-1,0,-1,-1 },{ 0,0,0,1,0,-1,1,-1 },{ 0,0,-1,0,1,0,1,1 },{ 0,0,0,-1,0,1,-1,1 } },
	{ { 0,0,-1,0,1,0,1,-1 },{ 0,0,0,-1,0,1,1,1 },{ 0,0,1,0,-1,0,-1,1 },{ 0,0,0,1,0,-1,-1,-1 } },
	{ { 0,0,1,0,0,-1,-1,-1 },{ 0,0,0,1,1,0,1,-1 },{ 0,0,-1,0,0,1,1,1 },{ 0,0,0,-1,-1,0,-1,1 } },
	{ { 0,0,-1,0,0,-1,1,-1 },{ 0,0,0,-1,1,0,1,1 },{ 0,0,1,0,0,1,-1,1 },{ 0,0,0,1,-1,0,-1,-1 } },
	{ { 0,0,-1,0,0,1,1,0 },{ 0,0,0,-1,-1,0,0,1 },{ 0,0,1,0,0,-1,-1,0 },{ 0,0,0,1,1,0,0,-1 } },
	{ { 0,0,0,-1,0,1,0,2 },{ 0,0,1,0,-1,0,-2,0 },{ 0,0,0,1,0,-1,0,-2 },{ 0,0,-1,0,1,0,2,0 } },
	{ { 0,0,0,1,-1,0,-1,1 },{ 0,0,-1,0,0,-1,-1,-1 },{ 0,0,0,-1,1,-0,1,-1 },{ 0,0,1,0,0,1,1,1 } }
};// 7种形状(长L| 短L| 反z| 正z| T| 直一| 田格)，4种朝向(上左下右)，8:每相邻的两个分别为x，y
 
 
class Tetris
{
public:
	const int blockType;   // 标记方块类型的序号 0~6
	int blockX;            // 旋转中心的x轴坐标
	int blockY;            // 旋转中心的y轴坐标
	int orientation;       // 标记方块的朝向 0~3
	const int(*shape)[8]; // 当前类型方块的形状定义
 
	int color;
 
	Tetris(int t, int color) : blockType(t), shape(blockShape[t]), color(color)
	{ }
 
	inline Tetris &set(int x = -1, int y = -1, int o = -1)
	{
		blockX = x == -1 ? blockX : x;
		blockY = y == -1 ? blockY : y;
		orientation = o == -1 ? orientation : o;
		return *this;
	}
 
	// 判断当前位置是否合法
	inline bool isValid(int x = -1, int y = -1, int o = -1)
	{
		x = x == -1 ? blockX : x;
		y = y == -1 ? blockY : y;
		o = o == -1 ? orientation : o;
		if (o < 0 || o > 3)
			return false;
 
		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++)
		{
			tmpX = x + shape[o][2 * i];
			tmpY = y + shape[o][2 * i + 1];
			if (tmpX < 1 || tmpX > MAPWIDTH ||
				tmpY < 1 || tmpY > MAPHEIGHT ||
				gridInfo[color][tmpY][tmpX] != 0)
				return false;
		}
		return true;
	}
 
	// 判断是否落地
	inline bool onGround()
	{
		if (isValid() && !isValid(-1, blockY - 1))
			return true;
		return false;
	}
 
	// 将方块放置在场地上
	inline bool place()
	{
		if (!onGround())
			return false;
 
		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++)
		{
			tmpX = blockX + shape[orientation][2 * i];
			tmpY = blockY + shape[orientation][2 * i + 1];
			gridInfo[color][tmpY][tmpX] = 2;
		}
		return true;
	}
 
	//撤销上一部操作
	inline void retract()
	{
		for (int y = 1; y <= MAPHEIGHT; y++)
			for (int x = 1; x <= MAPWIDTH; x++)
			{
				if(gridInfo[color][y][x] == 2)
					gridInfo[color][y][x] = 0;
			}
	}

	// 检查能否逆时针旋转自己到o
	inline bool rotation(int o)
	{
		if (o < 0 || o > 3)
			return false;
 
		if (orientation == o)
			return true;
 
		int fromO = orientation;
		while (true)
		{
			if (!isValid(-1, -1, fromO))
				return false;
 
			if (fromO == o)
				break;
 
			fromO = (fromO + 1) % 4;
		}
		return true;
	}
};
 
// 围一圈护城河
void init()
{
	int i;
	for (i = 0; i < MAPHEIGHT + 2; i++)
	{
		gridInfo[1][i][0] = gridInfo[1][i][MAPWIDTH + 1] = -2;
		gridInfo[0][i][0] = gridInfo[0][i][MAPWIDTH + 1] = -2;
	}
	for (i = 0; i < MAPWIDTH + 2; i++)
	{
		gridInfo[1][0][i] = gridInfo[1][MAPHEIGHT + 1][i] = -2;
		gridInfo[0][0][i] = gridInfo[0][MAPHEIGHT + 1][i] = -2;
	}
}
 
namespace Util
{
 
	// 检查能否从场地顶端直接落到当前位置
	inline bool checkDirectDropTo(int color, int blockType, int x, int y, int o)
	{
		const int *def = blockShape[blockType][o];
		for (; y <= MAPHEIGHT; y++)
			for (int i = 0; i < 4; i++)
			{
				int _x = def[i * 2] + x, _y = def[i * 2 + 1] + y;
				if (_y > MAPHEIGHT)
					continue;
				if (_y < 1 || _x < 1 || _x > MAPWIDTH || gridInfo[color][_y][_x])
					return false;
			}
		return true;
	}
 
	// 消去行
	void eliminate(int color)
	{
		int &count = transCount[color] = 0;
		int i, j, emptyFlag, fullFlag;
		maxHeight[color] = MAPHEIGHT;
		for (i = 1; i <= MAPHEIGHT; i++)
		{
			emptyFlag = 1;
			fullFlag = 1;
			for (j = 1; j <= MAPWIDTH; j++)
			{
				if (gridInfo[color][i][j] == 0)
					fullFlag = 0;
				else
					emptyFlag = 0;
			}
			if (fullFlag)
			{
				for (j = 1; j <= MAPWIDTH; j++)
				{
					// 注意这里只转移以前的块，不包括最后一次落下的块（“撤销最后一步”）
					trans[color][count][j] = gridInfo[color][i][j] == 1 ? 1 : 0;
					gridInfo[color][i][j] = 0;
				}
				count++;
			}
			else if (emptyFlag)
			{
				maxHeight[color] = i - 1;
				break;
			}
			else
				for (j = 1; j <= MAPWIDTH; j++)
				{
					gridInfo[color][i - count][j] =
						gridInfo[color][i][j] > 0 ? 1 : gridInfo[color][i][j];
					if (count)
						gridInfo[color][i][j] = 0;
				}
		}
		maxHeight[color] -= count;
		elimTotal[color] += elimBonus[count];
	}
 
	// 转移双方消去的行，返回-1表示继续，否则返回输者
	int transfer()
	{
		int color1 = 0, color2 = 1;
		if (transCount[color1] == 0 && transCount[color2] == 0)
			return -1;
		if (transCount[color1] == 0 || transCount[color2] == 0)
		{
			if (transCount[color1] == 0 && transCount[color2] > 0)
				swap(color1, color2);
			int h2;
			maxHeight[color2] = h2 = maxHeight[color2] + transCount[color1];
			if (h2 > MAPHEIGHT)
				return color2;
			int i, j;
 
			for (i = h2; i > transCount[color1]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = gridInfo[color2][i - transCount[color1]][j];
 
			for (i = transCount[color1]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = trans[color1][i - 1][j];
			return -1;
		}
		else
		{
			int h1, h2;
			maxHeight[color1] = h1 = maxHeight[color1] + transCount[color2];//从color1处移动count1去color2
			maxHeight[color2] = h2 = maxHeight[color2] + transCount[color1];
 
			if (h1 > MAPHEIGHT) return color1;
			if (h2 > MAPHEIGHT) return color2;
 
			int i, j;
			for (i = h2; i > transCount[color1]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = gridInfo[color2][i - transCount[color1]][j];
 
			for (i = transCount[color1]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = trans[color1][i - 1][j];
 
			for (i = h1; i > transCount[color2]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color1][i][j] = gridInfo[color1][i - transCount[color2]][j];
 
			for (i = transCount[color2]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color1][i][j] = trans[color2][i - 1][j];
 
			return -1;
		}
	}
 
	// 颜色方还能否继续游戏
	inline bool canPut(int color, int blockType)
	{
		Tetris t(blockType, color);
		for (int y = MAPHEIGHT; y >= 1; y--)
			for (int x = 1; x <= MAPWIDTH; x++)
				for (int o = 0; o < 4; o++)
				{
					t.set(x, y, o);
					if (t.isValid() && checkDirectDropTo(color, blockType, x, y, o))
						return true;
				}
		return false;
	}
 
	// 打印场地用于调试
	inline void printField()
	{
#ifndef _BOTZONE_ONLINE
		static const char *i2s[] = {
			"~~",
			"~~",
			"  ",
			"[]",
			"##"
		};
		cout << "~~：墙，[]：块，##：新块" << endl;
		for (int y = MAPHEIGHT + 1; y >= 0; y--)
		{
			for (int x = 0; x <= MAPWIDTH + 1; x++)
				cout << i2s[gridInfo[0][y][x] + 2];
			for (int x = 0; x <= MAPWIDTH + 1; x++)
				cout << i2s[gridInfo[1][y][x] + 2];
			cout << endl;
		}
#endif
	}
}
double CountPoints(Tetris & block,int x,int y,int o,int currBotColor)
 {
	 int ImagineFullLines=0;
					for (int i = 1; i <= MAPHEIGHT; i++)
					{
						int fullFlag = 1;
						for (int j = 1; j <= MAPWIDTH; j++)
						{
							if (gridInfo[currBotColor][i][j] == 0)
								fullFlag = 0;
						}
						if(fullFlag)
							ImagineFullLines++;
					}
					int ImagineHoles=0;
					for (int i = 1; i <= MAPHEIGHT; i++)
					{
						for (int j = 1; j <= MAPWIDTH; j++)
						{
							bool holeflag=false;
							for(int k=i+1;k<=MAPHEIGHT;k++)
							{
								if(gridInfo[currBotColor][k][j]!=0)
									holeflag=true;
							}
							if (gridInfo[currBotColor][i][j] == 0&&holeflag==true&&i<MAPHEIGHT)
								ImagineHoles++;
						}
					}
					
					int ImagineHeight=y;

					int ColomnTransitions=0;
					for (int i = 1; i <= MAPHEIGHT; i++)
					{
						for (int j = 1; j <= MAPWIDTH; j++)
						{
							if (gridInfo[currBotColor][i][j] == 0&&gridInfo[currBotColor][i+1][j]!=0&&i<MAPHEIGHT)
								ColomnTransitions++;
						}
					}
					int LineTransitions=0;
					for (int i = 1; i <= MAPHEIGHT; i++)
					{
						for (int j = 1; j <= MAPWIDTH; j++)
						{
							if (gridInfo[currBotColor][i][j] == 0&&((gridInfo[currBotColor][i][j+1]!=0&&j+1<=MAPWIDTH)||(gridInfo[currBotColor][i][j-1]!=0&&j-1>=1)))
								LineTransitions++;
							else if (gridInfo[currBotColor][i][j] != 0&&((gridInfo[currBotColor][i][j+1]==0&&j+1<=MAPWIDTH)||(gridInfo[currBotColor][i][j-1]==0&&j-1>=1)))
								LineTransitions++;
						}
					}
					int wells=0;
					for (int i = 1; i <= MAPHEIGHT; i++)
					{
						for (int j = 1; j <= MAPWIDTH; j++)
						{
							if (gridInfo[currBotColor][i][j] == 0&&gridInfo[currBotColor][i][j+1]!=0&&gridInfo[currBotColor][i][j-1]!=0&&i<MAPHEIGHT&&j!=1&&j!=MAPWIDTH)
								wells++;
						}
					}
					double totalpoints=ImagineFullLines*(3.4181268101392694)*2+ImagineHoles*(-7.899265427351652)+ImagineHeight*(-4.500158825082766)+wells*(-3.3855972247263626)+ColomnTransitions*(-9.348695305445199)+LineTransitions*(-3.2178882868487753);
					return totalpoints;
 }
 
int main()
{
	// 加速输入
	istream::sync_with_stdio(false);
	srand(time(NULL));
	init();
 
	int turnID, blockType;
	int nextTypeForColor[2];
 
	// 读入JSON
	string str;
	getline(cin, str);
	Json::Reader reader;
	Json::Value input;
	reader.parse(str, input);
 
	// 先读入第一回合，得到自己的颜色
	// 双方的第一块肯定是一样的
	turnID = input["responses"].size() + 1;
	auto &first = input["requests"][(Json::UInt) 0];
 
	blockType = first["block"].asInt();
	currBotColor = first["color"].asInt();
 
	enemyColor = 1 - currBotColor;
	nextTypeForColor[0] = blockType;
	nextTypeForColor[1] = blockType;
	typeCountForColor[0][blockType]++;
	typeCountForColor[1][blockType]++;
 
 
	// 然后分析以前每回合的输入输出，并恢复状态
	// 循环中，color 表示当前这一行是 color 的行为
	// 平台保证所有输入都是合法输入
	for (int i = 1; i < turnID; i++)
	{
		int currTypeForColor[2] = { nextTypeForColor[0], nextTypeForColor[1] };
		int x, y, o;
		// 根据这些输入输出逐渐恢复状态到当前回合
 
		// 先读自己的输出，也就是自己的行为
		// 自己的输出是一个序列，但是只有最后一步有用
		// 所以只保留最后一步
		// 然后模拟最后一步放置块
		auto &myOutput = input["responses"][i - 1];
		blockType = myOutput["block"].asInt();
		x = myOutput["x"].asInt();
		y = myOutput["y"].asInt();
		o = myOutput["o"].asInt();
 
		// 我当时把上一块落到了 x y o！
		Tetris myBlock(currTypeForColor[currBotColor], currBotColor);
		myBlock.set(x, y, o).place();
 
		// 我给对方什么块来着？
		typeCountForColor[enemyColor][blockType]++;
		nextTypeForColor[enemyColor] = blockType;
 
		// 然后读自己的输入，也就是对方的行为
		// 裁判给自己的输入是对方的最后一步
		auto &myInput = input["requests"][i];
		blockType = myInput["block"].asInt();
		x = myInput["x"].asInt();
		y = myInput["y"].asInt();
		o = myInput["o"].asInt();
 
		// 对方当时把上一块落到了 x y o！
		Tetris enemyBlock(currTypeForColor[enemyColor], enemyColor);
		enemyBlock.set(x, y, o).place();
 
		// 对方给我什么块来着？
		typeCountForColor[currBotColor][blockType]++;
		nextTypeForColor[currBotColor] = blockType;
 
		// 检查消去
		Util::eliminate(0);
		Util::eliminate(1);
 
		// 进行转移
		Util::transfer();
	}
 
 
	int blockForEnemy, finalX, finalY, finalO;
	
 
	// 做出决策（你只需修改以下部分）
	
	// 遇事不决先输出（平台上运行不会输出）
	Util::printField();
 
	// 贪心决策
	// 从下往上以各种姿态找到位置，要求能够直着落下
	Tetris block(nextTypeForColor[currBotColor], currBotColor);
	double maxpoints=-10000.0;
	
	
	for (int y = 1; y <=MAPHEIGHT; y++)
		for (int x = 1; x <= MAPWIDTH; x++)
			for (int o = 0; o < 4; o++)
			{
				if (block.set(x, y, o).isValid() &&
					Util::checkDirectDropTo(currBotColor, block.blockType, x, y, o)&&block.set(x,y,o).onGround()==true)
				{
					block.set(x,y,o).place();
					double totalpoints=CountPoints(block,x,y,o,currBotColor);
					if(totalpoints>maxpoints)
					{
						maxpoints=totalpoints;
						finalX = x;
						finalY = y;
						finalO = o;
					}
					block.set(x,y,o).retract();
				}
			}
 
			Tetris enemyblock(nextTypeForColor[enemyColor], enemyColor);
			int enemyX,enemyY,enemyO;
	for (int y = 1; y <=MAPHEIGHT; y++)
		for (int x = 1; x <= MAPWIDTH; x++)
			for (int o = 0; o < 4; o++)
			{
				if (enemyblock.set(x, y, o).isValid() &&
					Util::checkDirectDropTo(enemyColor, enemyblock.blockType, x, y, o))
				{
					enemyblock.set(x,y,o).place();
					double totalpoints=CountPoints(enemyblock,x,y,o,enemyColor);
					if(totalpoints>maxpoints)
					{
						maxpoints=totalpoints;
						enemyX = x;
						enemyY = y;
						enemyO = o;
					}
					enemyblock.set(x,y,o).retract();
				}
			}
			block.set(finalX,finalY,finalO).place();
	enemyblock.set(enemyX,enemyY,enemyO).place();
	// 检查消去
		Util::eliminate(0);
		Util::eliminate(1);
 
		// 进行转移
		Util::transfer();

	// 再看看给对方什么好
 
	int maxCount = 0, minCount = 99;
	for (int i = 0; i < 7; i++)
	{
		if (typeCountForColor[enemyColor][i] > maxCount)
			maxCount = typeCountForColor[enemyColor][i];
		if (typeCountForColor[enemyColor][i] < minCount)
			minCount = typeCountForColor[enemyColor][i];
	}
	if (maxCount - minCount == 2)
	{
		// 危险，找一个不是最大的块给对方吧
		
		
		double minpoints=10000.0;
		int ImagineType;
		for(ImagineType=0;ImagineType<7;ImagineType++)
		{
			if(typeCountForColor[enemyColor][ImagineType]==maxCount)continue;
			double Imaginemaxpoints=-10000000.0;
			Tetris ImagineBlock(ImagineType,enemyColor);
			for (int y = 1; y <= MAPHEIGHT; y++)
			for (int x = 1; x <= MAPWIDTH; x++)
			for (int o = 0; o < 4; o++)
			{
				if (ImagineBlock.set(x, y, o).isValid() &&
					Util::checkDirectDropTo(enemyColor, ImagineBlock.blockType, x, y, o))
				{
					ImagineBlock.set(x,y,o).place();
					double totalpoints=CountPoints(ImagineBlock,x,y,o,enemyColor);
					if(totalpoints>Imaginemaxpoints)
					{
						Imaginemaxpoints=totalpoints;
					}
					ImagineBlock.set(x,y,o).retract();
				}
			}
			if(Imaginemaxpoints<minpoints)
			{
				minpoints=Imaginemaxpoints;
				blockForEnemy=ImagineType;
			}
		}
		
	}
	else
	{
		
		
		double minpoints=10000.0;
		int ImagineType;
		for(ImagineType=0;ImagineType<7;ImagineType++)
		{
			double Imaginemaxpoints=-10000000.0;
			Tetris ImagineBlock(ImagineType,enemyColor);
			for (int y = 1; y <= MAPHEIGHT; y++)
			for (int x = 1; x <= MAPWIDTH; x++)
			for (int o = 0; o < 4; o++)
			{
				if (ImagineBlock.set(x, y, o).isValid() &&
					Util::checkDirectDropTo(enemyColor, ImagineBlock.blockType, x, y, o))
				{
					ImagineBlock.set(x,y,o).place();
					double totalpoints=CountPoints(ImagineBlock,x,y,o,enemyColor);
					if(totalpoints>Imaginemaxpoints)
					{
						Imaginemaxpoints=totalpoints;
					}
					ImagineBlock.set(x,y,o).retract();
				}
			}
			if(Imaginemaxpoints<minpoints)
			{
				minpoints=Imaginemaxpoints;
				blockForEnemy=ImagineType;
			}
		}
	}
 
	// 决策结束，输出结果（你只需修改以上部分）
 
	Json::Value output;
	Json::FastWriter writer;
 
	output["response"]["block"] = blockForEnemy;
 
	output["response"]["x"] = finalX;
	output["response"]["y"] = finalY;
	output["response"]["o"] = finalO;
 
	cout << writer.write(output);
 
	return 0;
}
