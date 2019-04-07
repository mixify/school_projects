/*
 * Class that defines the agent function.
 *
 * Written by James P. Biagioni (jbiagi1@uic.edu)
 * for CS511 Artificial Intelligence II
 * at The University of Illinois at Chicago
 *
 * Last modified 2/19/07
 *
 * DISCLAIMER:
 * Elements of this application were borrowed from
 * the client-server implementation of the Wumpus
 * World Simulator written by Kruti Mehta at
 * The University of Texas at Arlington.
 *
 */

import java.util.ArrayList;
import java.util.Random;

class AgentFunction {

	private static final int INIT = -1;
//	private static final int CERTAINLY_THERE = 10000;
//	private static final int UNCERTAIN = 1;
//	private static final int CERTAINLY_NOT_THERE = 0;

	private static final int WUMPUS = 0;
	private static final int PIT = 1;
	private static final int GOLD = 2;

	private boolean turning;
	// string to store the agent's name
	// do not remove this variable
	private String agentName = "Agent Smith";

	// all of these variables are created and used
	// for illustration purposes; you may delete them
	// when implementing your own intelligent agent
	private int[] actionTable;

	private int[][][] worldMemory;
	private int[][] boundary;
	private int[] location;
	private int[] wumpusLocation;
	private boolean isitfirst=true;
	private boolean wumpusAlive;
	private boolean wumpusCertained;
	
	private char targetDirection;
	private ArrayList<int[]> visitList;

	private boolean bump;
	private boolean glitter;
	private boolean breeze;
	private boolean stench;
	private boolean scream;
	private Random rand;

	public AgentFunction(int worldSize, int[] location, char direction)
	{
		// for illustration purposes; you may delete all code
		// inside this constructor when implementing your
		// own intelligent agent

		// this integer array will store the agent actions
		actionTable = new int[8];
		actionTable[0] = Action.GO_FORWARD;
		actionTable[1] = Action.GO_FORWARD;
		actionTable[2] = Action.GO_FORWARD;
		actionTable[3] = Action.GO_FORWARD;
		actionTable[4] = Action.TURN_RIGHT;
		actionTable[5] = Action.TURN_LEFT;
		actionTable[6] = Action.GRAB;
		actionTable[7] = Action.SHOOT;


		
		worldMemory = new int[7][7][3];
		for (int i = 0; i < 7; i++) {
			for (int j = 0; j < 7; j++) {
				for (int k = 0; k < 3; k++) {//0 wumpus 1 pit
					if(k == 2)
						worldMemory[i][j][k] = 1;
					else
						worldMemory[i][j][k] = INIT;
				}
			}
		}
		boundary = new int[2][2];

		boundary[0][0] = -1;
		boundary[0][1] = 10000;
		boundary[1][0] = -1;
		boundary[1][1] = 10000;

		this.location = new int[2];
		this.location[0] = 3;
		this.location[1] = 3;

		wumpusLocation = new int[2];
		wumpusLocation[0] = -1;
		wumpusLocation[1] = -1;
		wumpusAlive = true;
		wumpusCertained = false;
		turning = false;

		visitList = new ArrayList<>();
		rand = new Random();
		turning = false;


	}

	public int process(TransferPercept tp)
	{
		if(!turning)//not turning... moves or shoot??....
		{
			if(!visited(location[0], location[1]))
			{
				int[] cur_location = new int[2];
				cur_location[0] = location[0];
				cur_location[1] = location[1];
				visitList.add(cur_location);
				if(tp.getGlitter())
					return actionTable[6];
				else
					updateMemory(GOLD, false);
				if(tp.getBump())
				{
					switch(tp.getDirection())
					{
					case 'W':
						location[0]++;
						boundary[0][0] = location[0];
						boundary[0][1] = location[0] + 4;
						break;
					case 'E':
						location[0]--;
						boundary[0][1] = location[0];
						boundary[0][0] = location[0] - 4;
						break;
					case 'N':
						location[1]--;
						boundary[1][1] = location[1];
						boundary[1][0] = location[1] - 4;
						break;
					case 'S':
						location[1]++;
						boundary[1][0] = location[1];
						boundary[1][1] = location[1] + 4;
						break;
					}
				}
				if(tp.getScream())
				{
					wumpusAlive = false;
					for (int i = 0; i < 7; i++) {
						for (int j = 0; j < 7; j++) {
							worldMemory[i][j][WUMPUS] = 0;
						}
					}
				}
				if(tp.getStench())
					updateMemory(WUMPUS, true);
				else
					updateMemory(WUMPUS, false);
				if(tp.getBreeze())
					updateMemory(PIT, true);
				else
					updateMemory(PIT, false);


			}
			targetDirection = chooseDirection();
			turning = true;
		}
		char direction = tp.getDirection();
		boolean isArrow = tp.isArrow();
		if(wumpusKillable(isArrow, direction))
		{
			switch(direction)
			{
			case 'W':
				for (int i = location[0]-1 ; i > 0; i--) 
					worldMemory[i][location[1]][WUMPUS] = 0;
				break;
			case 'E':
				for (int i = location[0]+1 ; i < 7; i++) 
					worldMemory[i][location[1]][WUMPUS] = 0;
				break;
			case 'N':
				for (int i = location[1]+1 ; i < 7; i++) 
					worldMemory[location[0]][i][WUMPUS] = 0;
				break;
			case 'S':
				for (int i = location[1]-1 ; i > 0; i--) 
					worldMemory[location[0]][i][WUMPUS] = 0;
				break;
			}
			return actionTable[7];
		}
		if(direction == targetDirection)
		{
			turning = false;
			switch(direction)
			{
			case 'W':
				location[0]--;
				break;
			case 'E':
				location[0]++;
				break;
			case 'N':
				location[1]++;
				break;
			case 'S':
				location[1]--;
				break;
			}
			return actionTable[0];
		}
		else//keep turning~~~
		{
			if(rightIsBetter(direction,targetDirection))
				return actionTable[4];
			else
				return actionTable[5];
		}


	}

	private void updateMemory(int type, boolean check)
	{
		int x = location[0];
		int y = location[1];

		if(type == GOLD)
			worldMemory[x][y][type] = 0;
		else
			if(check == true)//here has that percept
			{
				worldMemory[x][y][type] = 0;
				if (x-1 >= 0) checkAndUpdateRisk(x-1,y,type,check);
				if (x+1 < 7) checkAndUpdateRisk(x+1,y,type,check);
				if (y-1 >= 0) checkAndUpdateRisk(x,y-1,type,check);
				if (y+1 < 7) checkAndUpdateRisk(x,y+1,type,check);
			}
			else//no percept here.. now update safe places..
			{
				worldMemory[x][y][type] = 0;
				if (x-1 >= 0) checkAndUpdateRisk(x-1,y,type,check);
				if (x+1 < 7) checkAndUpdateRisk(x+1,y,type,check);
				if (y-1 >= 0) checkAndUpdateRisk(x,y-1,type,check);
				if (y+1 < 7) checkAndUpdateRisk(x,y+1,type,check);
			}
	}


	private void checkAndUpdateRisk(int x, int y, int type, boolean check)
	{
		if(check)
		{
			if(type == WUMPUS && (wumpusCertained || !wumpusAlive))
				return;
			if(worldMemory[x][y][type]==INIT)
				worldMemory[x][y][type] = 1;//hmm.. risky..
			else if(worldMemory[x][y][type] > 0 && worldMemory[x][y][type] < 3)
			{
				worldMemory[x][y][type]++;//more and more risky...
				if(worldMemory[x][y][type] == 3)//definitely risky
				{
					if(type == WUMPUS)
					{

						for (int i = 0; i < 7; i++)
							for (int j = 0; j < 7; j++) 
								worldMemory[i][j][WUMPUS] = 0;
						worldMemory[x][y][WUMPUS] = 3;
						wumpusCertained = true;
						wumpusLocation[0] = x;
						wumpusLocation[1] = y;
					}
				}
			}
		}
		else
			worldMemory[x][y][type] = 0;//safe
	}

	private void checkAndAddtoList(int x, int y,char direction, ArrayList<Character> list)
	{
		int i = 48;
		if(visited(x,y))
			i-=12;

		for (; i > 0 ; i-=((worldMemory[x][y][WUMPUS]+worldMemory[x][y][PIT])*24+1))
		{
			if(worldMemory[x][y][WUMPUS]+worldMemory[x][y][PIT] < 0)
				break;
			else if(worldMemory[x][y][WUMPUS] == 3 || worldMemory[x][y][PIT] == 3 )
				break;
			list.add(direction);
		}
		return;
	}

	private boolean wumpusKillable(boolean isArrow, char direction)
	{
		if(!wumpusAlive || !isArrow)
			return false;
		else
		{
			int sum = 0;
			System.out.println("direction = " + direction);
			switch(direction)
			{
			case 'W':
				for (int i = location[0]-1 ; i > 0; i--) 
				{
					if(worldMemory[i][location[1]][WUMPUS] == 3)
						return true;
					else if(worldMemory[i][location[1]][WUMPUS] == INIT)
						continue;
					sum+=worldMemory[i][location[1]][WUMPUS] * worldMemory[i][location[1]][WUMPUS] * worldMemory[i][location[1]][WUMPUS];
				}
				break;
			case 'E':
				for (int i = location[0]+1 ; i < 7; i++) 
				{
					if(worldMemory[i][location[1]][WUMPUS] == 3)
						return true;
					else if(worldMemory[i][location[1]][WUMPUS] == INIT)
						continue;
					sum+=worldMemory[i][location[1]][WUMPUS] * worldMemory[i][location[1]][WUMPUS] * worldMemory[i][location[1]][WUMPUS];
				}
				break;
			case 'N':
				for (int i = location[1]+1 ; i < 7; i++) 
				{
					if(worldMemory[location[0]][i][WUMPUS] == 3)
						return true;
					else if(worldMemory[location[0]][i][WUMPUS] == INIT)
						continue;
					sum+=worldMemory[location[0]][i][WUMPUS] * worldMemory[location[0]][i][WUMPUS] * worldMemory[location[0]][i][WUMPUS];
				}
				break;
			case 'S':
				for (int i = location[1]-1 ; i > 0; i--) 
				{
					if(worldMemory[location[0]][i][WUMPUS] == 3)
						return true;
					else if(worldMemory[location[0]][i][WUMPUS] == INIT)
						continue;
					sum+=worldMemory[location[0]][i][WUMPUS] * worldMemory[location[0]][i][WUMPUS] * worldMemory[location[0]][i][WUMPUS];
				}
				break;
			}
			ArrayList<Boolean> TFList = new ArrayList<>();
			for (int i = 0; i < sum; i++) {
				TFList.add(true);
			}
			for (int i = 0; i < 4; i++) {
				TFList.add(false);
			}
			return TFList.get(rand.nextInt(TFList.size()));
		}

	}
	public String getAgentName() {
		return agentName;
	}

	private char chooseDirection()
	{
		ArrayList<Character> directionList = new ArrayList<>();
		int x = location[0];
		int y = location[1];

		if (x-1 >= 0 && x-1 >= boundary[0][0]) checkAndAddtoList(x-1,y,'W',directionList);
		if (x+1 < 7 && x+1 <= boundary[0][1]) checkAndAddtoList(x+1,y,'E',directionList);
		if (y-1 >= 0 && y-1 >= boundary[1][0]) checkAndAddtoList(x,y-1,'S',directionList);
		if (y+1 < 7 && y+1 <= boundary[1][1]) checkAndAddtoList(x,y+1,'N',directionList);

		return directionList.get(rand.nextInt(directionList.size()));
	}

	private boolean visited(int x, int y)
	{
		for (int i = 0; i < visitList.size(); i++) {
			if(x == visitList.get(i)[0] && y == visitList.get(i)[1])
			{
				return true;
			}
		}
		return false;
	}

	private boolean rightIsBetter(char curDirection, char targetDirection)
	{
		if(curDirection == 'W' && targetDirection == 'N')
			return true;
		else if(curDirection == 'N' && targetDirection == 'E')
			return true;
		else if(curDirection == 'E' && targetDirection == 'S')
			return true;
		else if(curDirection == 'S' && targetDirection == 'W')
			return true;
		else
			return false;
	}
}
