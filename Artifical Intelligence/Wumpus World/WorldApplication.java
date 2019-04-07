/*
 * Wumpus-Lite, version 0.21 alpha
 * A lightweight Java-based Wumpus World Simulator
 *
 * Written by James P. Biagioni (jbiagi1@uic.edu)
 * for CS511 Artificial Intelligence II
 * at The University of Illinois at Chicago
 *
 * Thanks to everyone who provided feedback and
 * suggestions for improving this application,
 * especially the students from Professor
 * Gmytrasiewicz's Spring 2007 CS511 class.
 *
 * Last modified 4/14/08
 *
 * DISCLAIMER:
 * Elements of this application were borrowed from
 * the client-server implementation of the Wumpus
 * World Simulator written by Kruti Mehta at
 * The University of Texas at Arlington.
 *
 */


import java.io.*;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


class Clause implements Cloneable{ 
	public static final Clause EMPTY = new Clause();
	Set<Literal> literals = new LinkedHashSet<Literal>();
	ArrayList<Literal> literalList;
	
	private int cachedHashCode = -1;
	int count;
	int bit;
	
	AssignTable at;
	public Clause()
	{
		this(new ArrayList<Literal>());
	}
//	public Clause(ArrayList<Literal> literals) {
//		this.literalList = literals;
//		this.literals = new LinkedHashSet<>(literals);
//		this.count = literals.size();
//	}

	public void setAt(AssignTable at) {
		this.at = at;
	}

	public boolean isTrue()
	{
		for (Literal literal : literalList) {
			if(at.getTF(literal))
				return true;
		}
		return false;
	}
	public Clause(Collection<Literal> literals)
	{
		for(Literal l : literals)
			this.literals.add(l);
		this.literalList = new ArrayList<Literal>(literals);
		this.count = literals.size();
	}
	public Clause(ArrayList<Literal> literals, int bit) {
		this.literalList = literals;
		this.literals = new LinkedHashSet<>(literals);
		this.count = literals.size();
		this.bit = bit;
	}
//	public Clause(Set<Literal> literals) {
//		this.literals = literals;
//		this.literalList = new ArrayList<>(literals);
//		this.count = literals.size();
//	}
	@Override
	public String toString() {
		// TODO Auto-generated method stub
		String str = "";
		int i = 0;
		for(Literal l : literals)
		{
			str = str + l.toString() +((i!=literals.size()-1)?"V":"");
			i++;
		}
		return str;
	}
	
	public void removeLiteral(int i)
	{
		literals.remove(i);
		this.count--;
	}
	
	public void removeLiteral(Literal l)
	{
		literals.remove(l);
		this.count--;
	}
	public Set<Literal> getLiterals() {
		return this.literals;
	}
	public Clause joinClause(Clause c)
	{
		Set<Literal> newLiteral = new LinkedHashSet<>();
		newLiteral.addAll(this.literals);
		newLiteral.addAll(c.literals);
		return new Clause(newLiteral);
	}
	
	public ArrayList<String> intersect(Clause c)
	{
		ArrayList<String> symbols = new ArrayList<>();
		for (Literal c1l : literals) {
			if(!c1l.is_negated)
			{
				for (Literal c2l : c.getLiterals()) {
					if(c2l.is_negated)
					{
						if(c1l.varname.equals(c2l.varname))
							symbols.add(c1l.varname);
					}
				}
			}
		}
		return symbols;
	}
	protected Object clone() throws CloneNotSupportedException {
		// TODO Auto-generated method stub
		return super.clone();
	}
	
	public boolean hasSymbol(String symbol)
	{
		for (Literal literal : literals) {
			if(literals.size() == 1)
				return true;
			else if(literal.varname.equals(symbol))
				return true;
		}
		return false;
	}
	@Override
	public int hashCode() {
		// TODO Auto-generated method stub
		if(cachedHashCode == -1)
			cachedHashCode = literals.hashCode();
		return cachedHashCode;
	}
	@Override
	public boolean equals(Object obj) {
		if(null == obj)
			return false;
		if(this == obj)
			return true;
		if(!(obj instanceof Clause))
			return false;
		Clause c = (Clause) obj;
		
		return c.literals.equals(this.literals);
		// TODO Auto-generated method stub
	}
	
	public Literal getRandomLiteral()
	{
		Random r = new Random();
		return literalList.get(r.nextInt(literalList.size()));
	}
	public boolean discardable()
	{
		ArrayList<Literal> literalList = new ArrayList<Literal>(literals);
		for (int i = 0; i < literalList.size()-1; i++) {
			Literal li = literalList.get(i);
			for (int j = i+1; j < literalList.size(); j++) {
				Literal lj = literalList.get(j);
				if(li.isOpposite(lj))
					return true;
			}

		}
		return false;
	}
	public boolean reducing(Clause c1, Clause c2)
	{
		int bigger = (c1.literals.size()>c2.literals.size()?c1.literals.size():c2.literals.size());
		return this.literals.size() <= bigger;
	}
}
	//	}

class AssignTable{
	int symbolCount;
	String []symbols;
	boolean []TFtable;

	
	public AssignTable(ArrayList<String> symbols) {
		this.symbols = (String[]) symbols.toArray(new String[0]);
		this.symbolCount = symbols.size();
		TFtable = new boolean[symbolCount];
	}
	
	public boolean getTF(Literal l)
	{
		int index = findSymbolIndex(l.varname);
		if(l.is_negated)
			return !TFtable[index];
		else
			return TFtable[index];
	}
	
	public boolean getTF(String symbol)
	{
		int index = findSymbolIndex(symbol);
		return TFtable[index];
	}
	
	public boolean getTF(int i)
	{
		return TFtable[i];
	}

	public String getSymbol(int i)
	{
		return symbols[i];
	}
	public void setTFtable(int bits)
	{
		for (int i = 0; i < symbolCount; i++) {
			if((bits & 1) == 1)
				TFtable[i] = true;
			else
				TFtable[i] = false;
			
			bits = bits>>1;
		}
	}
	
	public void setTFtable(Literal l)
	{
		int idx = findSymbolIndex(l.varname);
		TFtable[idx] = !TFtable[idx];
	}

	public void printTable()//for Debug
	{
		for (int i = 0; i < TFtable.length; i++) {
			if(TFtable[i])
				if(i==TFtable.length-1)
					System.out.println("~"+symbols[i]);
				else
					System.out.print("~"+symbols[i]+"V");
			else
				if(i==TFtable.length-1)
					System.out.println(symbols[i]);
				else
					System.out.print(symbols[i]+"V");
		}
	}

	public Clause getClause(int bit)
	{
		ArrayList<Literal> literals = new ArrayList<>();
		for (int i = 0; i < TFtable.length; i++)
			literals.add(new Literal(symbols[i], TFtable[i]));
		Clause clause = new Clause(literals, bit);
		return clause;
	}
	public int findSymbolIndex(String str)
	{
		for (int i = 0; i < symbols.length; i++) {
			if(str.equals(symbols[i]))
				return i;
		}
		return -1;
	}
}
class Query {
	String sentence;
	AssignTable at;
	public Query(String sentence, AssignTable at) {
		this.sentence = sentence;
		this.at = at;
	}
	public boolean solve()
	{
//		System.out.println("Original = " + sentence);
		String replacestr = sentence.toString();
		for (int i = 0; i < at.symbolCount; i++) {
//			System.out.println("symbols => " + at.getSymbol(i) + " is " + at.getTF(i));
			String original = at.getSymbol(i);
			replacestr = replacestr.replaceAll(original, (at.getTF(i)?"T":"F"));
		}
//
//		System.out.println("First = " + sentence);

		replacestr = replacestr.replaceAll("~T", "F");
		replacestr = replacestr.replaceAll("~F", "T");
//		System.out.println("eli negative = " + replacestr);

		replacestr = replacestr.replaceAll("T\\^T", "T");
		replacestr = replacestr.replaceAll("T\\^F", "F");
		replacestr = replacestr.replaceAll("F\\^T", "F");
		replacestr = replacestr.replaceAll("F\\^F", "F");
//		System.out.println("eli and = " + replacestr);

		replacestr = replacestr.replaceAll("TVT", "T");
		replacestr = replacestr.replaceAll("FVT", "T");
		replacestr = replacestr.replaceAll("TVF", "T");
		replacestr = replacestr.replaceAll("FVF", "F");
//		System.out.println("eli or = " + replacestr);

		replacestr = replacestr.replaceAll("T=>T", "T");
		replacestr = replacestr.replaceAll("T=>F", "F");
		replacestr = replacestr.replaceAll("F=>T", "T");
		replacestr = replacestr.replaceAll("F=>F", "F");
//		System.out.println("eli imp = " + replacestr);

		replacestr = replacestr.replaceAll("T<=>T", "T");
		replacestr = replacestr.replaceAll("T<=>F", "F");
		replacestr = replacestr.replaceAll("F<=>T", "F");
		replacestr = replacestr.replaceAll("F<=>F", "T");
//		System.out.println("eli bi = " + replacestr);

		if(replacestr.equals("T")) return true; else return false;
	}
	public void print()
	{
		System.out.println(sentence);
	}
}
class Rule { 
	ArrayList<Literal> literals; 
	int count;

	int secondRule;

	AssignTable at;

	public static final int AND = 0;
	public static final int OR = 1;
	Rule(ArrayList<Literal> literals, int secondRule, AssignTable at)
	{
		this.literals = literals;
		this.count = literals.size();
		this.secondRule = secondRule;
		this.at = at;
	}
	
	public boolean isTrue()
	{
		if(literals.size() == 1)
		{
			Literal l = literals.get(0);
			return at.getTF(l);
		}
		else
		{
			Literal left = literals.get(0);
			if(at.getTF(left))//left true => right true means return true
			{
				if(secondRule == AND)
				{
					for (int i = 1; i < literals.size(); i++) {
						if(at.getTF(literals.get(i)) == false)//some of right symbol false
							return false;//and will be end as false
					}
					return true;
				}
				else//secondRule is or
				{
					for (int i = 1; i < literals.size(); i++) {
						if(at.getTF(literals.get(i)) == true)//some of right symbol true
							return true;//and will be end as true
					}
					return false;//all was false
				}
			}
			else//left false => right false means return true
			{
				if(secondRule == AND)//right end should be false or it'll be false in the end
				{
					for (int i = 1; i < literals.size(); i++) {
						if(at.getTF(literals.get(i)) == false)//some of right symbol false
							return true;//and will be end as false
					}
					return false;
				}
				else//secondRule is or => all should be false
				{
					for (int i = 1; i < literals.size(); i++) {
						if(at.getTF(literals.get(i)) == true)//some of right symbol true
							return false;//and will be end as true
					}
					return true;//all was false
				}
			}
		}
	}

	public boolean hasSymbol(String symbol)
	{
		for (Literal literal : literals) {
			if(literal.varname.equals(symbol))
				return true;
		}
		return false;
	}
	@Override
	public String toString() {
		if(literals.size() == 1)
			return literals.get(0).toString();
		else
		{
			String str = literals.get(0).toString()+"<=>";
			for (int i = 1; i < literals.size(); i++) {
				if(i==literals.size()-1)
					str = str + literals.get(i);
				else
					str = str + literals.get(i) + ((secondRule==AND)?"^":"V");
			}
			return str;
		}
		// TODO Auto-generated method stub
	}
	

	public int getCount()
	{
		return this.count;
	}
}//to make cnf...
class CNF{
	AssignTable at;
	ArrayList<Clause> cnfs;
	public CNF(AssignTable at, ArrayList<Clause> cnfs) {
		this.at = at;
		this.cnfs = cnfs;
		for (Clause clause : cnfs) {
			clause.setAt(at);
		}
	}
	public boolean isSAT()
	{
		for (Clause clause : cnfs) {
			if(!clause.isTrue())
				return false;
		}
		return true;
	}
	public Clause getRandomFalseClause()
	{
		Random r = new Random();
		ArrayList<Clause> falseClauses = new ArrayList<>();
		for (Clause clause : cnfs) {
			if(!clause.isTrue())
				falseClauses.add(clause);
		}
		return falseClauses.get(r.nextInt(falseClauses.size()));
	}
	public Literal getBestLiteral(Clause c)
	{
		int max = -1;
		int satCount;
		Literal bestLiteral = null;
		for (Literal l : c.getLiterals()) {
			satCount = 0;
			at.setTFtable(l);
			
			for (Clause clause : cnfs) {
				if(clause.isTrue())
					satCount++;
			}
			if(satCount > max)
			{
				max = satCount;
				bestLiteral = l;
			}
			at.setTFtable(l);
		}
		return bestLiteral;
	}
}
class Literal { 
	String varname; 
	boolean is_negated; 
	
	private int cachedHashCode = -1;
	Literal(String literal)
	{
//		System.out.println("literal = " + literal);
		if(literal.contains("~"))
		{
			this.varname = literal.substring(1, literal.length());
			this.is_negated = true;
		}
		else
		{
			this.varname = literal;
			this.is_negated = false;
		}
	}
	Literal(String literal, boolean is_negated)
	{
		this.varname = literal;
		this.is_negated = is_negated;
	}
	
	public boolean isOpposite(Literal l)
	{
		if(this.varname.equals(l.varname) && (this.is_negated != l.is_negated))
			return true;
		else
			return false;
	}

	public boolean isSame(Literal l)
	{
		if(this.varname.equals(l.varname) && (this.is_negated == l.is_negated))
			return true;
		else
			return false;
	}
	@Override
	public String toString() {
		// TODO Auto-generated method stub
		return (is_negated?"~":"")+varname;
	}
	
	public Literal getOpposite()
	{
		return new Literal(this.varname, !this.is_negated);
	}
	
	
//	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if(!(obj instanceof Literal))
			return false;
		Literal l = (Literal) obj;
		boolean ret = l.is_negated==this.is_negated && l.varname.equals(this.varname);
//		System.out.println(this.toString() + "and" + l.toString() + " : " + ret);
		return ret;
	}
	
	@Override
	public int hashCode() {
		if(cachedHashCode == -1)
		{
			cachedHashCode = 17;
			cachedHashCode = (cachedHashCode * 37)
							+ (!is_negated ?  "+".hashCode() : "-".hashCode());
			cachedHashCode = (cachedHashCode * 37) + varname.hashCode();
		}
		return cachedHashCode;
	}

}
class WorldApplication {


	private static String VERSION = "v0.21a";
	private static final int RESOLUTION = 1;
	private static final int WALKSAT = 2;
	String gameboard="";
	String query="";
	String KB="";
	String results="";

	static String []KBary;
	int algo;
	int worldSize;

	static AssignTable at;
	static ArrayList<Rule> rules;

	static Query qry;
	static String[] querySymbols;
	static String[] cnfSymbols;
	static Set<Clause> queryClause;
	
	static ArrayList<Clause> clauses;

	public static void main (String args[]) throws Exception {
		WorldApplication wa=new WorldApplication();
		Learning learning = new Learning();

		boolean nonDeterministicMode=false;

		if(wa.readPara(args)==6)
		{
			FileWriter fw = new FileWriter(wa.results);
			BufferedWriter bw = new BufferedWriter(fw); 
			char[][] wumpusWorld = readWumpusWorld(wa.worldSize,wa.gameboard);
		
			//			return;

			KBary = Learning.studyKB(wumpusWorld, wa.worldSize, wa.KB, bw);

			File file = new File(wa.query);
			BufferedReader br = new BufferedReader(new FileReader(file));
			String str;

			for (int i = 1; (str = br.readLine())!=null ; i++) {
				long start = System.currentTimeMillis();
				if(str.length() == 0)
					continue;
				String qry = str.substring(str.indexOf('.')+1);
				if(solveSAT(KBary,qry, wa.algo))
					bw.write(i + ".yes");
				else
					bw.write(i + ".no");
				long end = System.currentTimeMillis();

				System.out.println( "time : " + ( end - start )/1000.0 );


				bw.newLine();
				bw.flush();
			}

			br.close();
			bw.close();
			
//			return;
		}
		else
		{
			wa.usage();
		}
	}

	private static boolean solveSAT(String []KBary, String query, int algorithm) throws InterruptedException
	{
		ParsePL(KBary);
		clauses = new ArrayList<>();
		
		for (int i = 0; i < rules.size(); i++) 
		{
			convertKBCNF(rules.get(i));
		}
		
		ParsePL(query);

		if(algorithm == RESOLUTION)
		{
			convertQuery(qry, true);

			Set<Clause> clausesSet = new LinkedHashSet<>(clauses);

			Set<Clause> newcl = new LinkedHashSet<Clause>();

			int position = 0;
			do
			{
				int size = clausesSet.size();
//				System.out.println("size = " + size);
				List<Clause> clausesAsList = new ArrayList<Clause>(clausesSet);
				for (int i = 0; i < size-1; i++) {
					Clause ci = clausesAsList.get(i);
					for (int j = i+1; j < size; j++) 
					{
						Clause cj = clausesAsList.get(j);

						Set<Clause> resolvents = PLResolve(ci, cj,0);

						for(Clause c : resolvents)
						{
							//						System.out.println(ci.toString() + " and " + cj.toString() + " has : " + c.toString());
							if(c.literals.size() == 0)
							{
								//							if(queryClause.contains(ci) ||queryClause.contains(cj) )
								//							{
								return true;
								//							}
							}
						}
						newcl.addAll(resolvents);
					}
				}
				if(clausesSet.containsAll(newcl))
					return false;
				//			clauses.addAll(newcl);


				clausesSet.addAll(newcl);


			}while(true);

		}
		else if(algorithm==WALKSAT)
		{
			convertQuery(qry, false);

			Set<Clause> clausesSet = new LinkedHashSet<>(clauses);
			ArrayList<Clause> clausesasList = new ArrayList<>(clausesSet);
			Set<String> symbols = new LinkedHashSet<>();
//			p and b each has 16...???
			for (Clause clause : clausesSet) {
				Set<Literal> literals = clause.getLiterals();
				for (Literal literal : literals) {
					symbols.add(literal.varname);
				}
			}
//			System.out.println(symbols.size());
			ArrayList <String> symbolList = new ArrayList<String>(symbols);
			AssignTable at = new AssignTable(symbolList);
			CNF cnf = new CNF(at, clausesasList);
			Random r = new Random();
			int maxFlips = 100000;
			double p = 0.5;

			if(symbols.size() == 32)
				cnf.at.setTFtable(r.nextInt());
			for (int i = 0; i < maxFlips; i++) 
			{
				if(cnf.isSAT())
					return true;
				Clause c = cnf.getRandomFalseClause();
				if(r.nextDouble() > p)
					cnf.at.setTFtable(c.getRandomLiteral());
				else
				{
					Literal best = cnf.getBestLiteral(c);
					cnf.at.setTFtable(best);
				}
			}
			return false;
		}
		return false;
	}

	private static Set<Clause> PLResolve(Clause c1, Clause c2, int i)
	{
		Set<Clause> resolvents = new LinkedHashSet<Clause>();
		
		try {
			resolvePwithN(c1, c2, resolvents);
			resolvePwithN(c2, c1, resolvents);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		return resolvents;
	}
	
	private static void resolvePwithN(Clause c1, Clause c2, Set<Clause> resolvents) throws InterruptedException
	{
//		System.out.println("before");
//		System.out.println(c1.toString() + " and " + c2.toString());
		ArrayList<String> symbols = c1.intersect(c2);
		for(String symbol : symbols)
		{
			ArrayList<Literal> resolveLiterals = new ArrayList<>();
			
			for(Literal c1l : c1.getLiterals())
			{
				if(c1l.is_negated || !c1l.varname.equals(symbol))
					resolveLiterals.add(c1l);
			}

			for(Literal c2l : c2.getLiterals())
			{
				if(!c2l.is_negated || !c2l.varname.equals(symbol))
					resolveLiterals.add(c2l);
			}
			
			Clause resolvent = new Clause(resolveLiterals);
//			System.out.println(c1.toString() + " + " + c2.toString() + " = " + resolvent.toString());
//			Thread.sleep(1000);

//			System.out.println("adding = " + resolvent.toString());
			if(!resolvent.discardable())
				resolvents.add(resolvent);
		}
	}


	private static String ParsePL(String []formulas)//using truth table conversion
	{
		String CNF = "";
		String formula;

		final int AND = 0;
		final int OR = 1;

		int type = 0;
		rules = new ArrayList<>();

		for (int i = 0; i < formulas.length; i++)
		{
			String symbol = "";
			formula = formulas[i];

			ArrayList<String> symbols = new ArrayList<>();
			ArrayList<Literal> rulesLiteral = new ArrayList<>();
			if(!formula.contains("<=>"))
			{
				rulesLiteral.add(new Literal(formula));
				symbol = getSymbol(formula);
				symbols.add(symbol);
						
			}
			else
			{
				String[] biconds = formula.split("<=>");

				rulesLiteral.add(new Literal(biconds[0]));

				symbol = getSymbol(biconds[0]);
				symbols.add(symbol);

				String deleteBrackets = biconds[1].substring(1, biconds[1].length()-1);

				String[] orLiterals = deleteBrackets.split("V");
				String[] andLiterals = deleteBrackets.split("\\^");

				if(deleteBrackets.contains("V"))
				{
					type = OR;
					for (int j = 0; j < orLiterals.length; j++) {
						rulesLiteral.add(new Literal(orLiterals[j]));

						symbol = getSymbol(orLiterals[j]);
						symbols.add(symbol);
					}
				}
				else if(deleteBrackets.contains("^"))
				{
					type = AND;
					for (int j = 0; j < andLiterals.length; j++) {
						rulesLiteral.add(new Literal(andLiterals[j]));
						symbol = getSymbol(andLiterals[j]);
						symbols.add(symbol);
					}
				}

			}
			
			AssignTable at = new AssignTable(symbols);
			rules.add(new Rule(rulesLiteral, type, at));
		}

		return CNF;
	}
	
	private static String ParsePL(String formula)//using truth table conversion
	{
		String CNF = "";

		ArrayList<String> symbols = new ArrayList<>();
		
		for (int i = 0; i < formula.length(); i++) {
			if(formula.charAt(i) == 'B' || formula.charAt(i) == 'P')
				symbols.add(formula.substring(i, i+4));
		}
		AssignTable at = new AssignTable(symbols);
		qry = new Query(formula, at);

		return CNF;
	}

private static String getSymbol(String str)
	{
		String symbol;
		if(str.startsWith("~"))
			symbol = str.substring(1,str.length());
		else
			symbol = str;
		return symbol;
	}
	private static String convertKBCNF(Rule r) throws InterruptedException
	{
		int count = r.getCount();
		String CNF;
		long bitsMax = (long) Math.pow(2,count);
//		System.out.println(r.toString());
		ArrayList<Clause> tmpCnfs = new ArrayList<>();

		if(r.count == 1)
			clauses.add(new Clause(r.literals));
		else 
		{
			if(r.secondRule == Rule.OR)
			{
				for (int i = 0; i < r.count ; i++) {
					Set<Literal> lSet = new LinkedHashSet<>();
					if(i==0)
					{
						for (int j = 0; j < r.count; j++) {
							if(j==0)
								lSet.add(r.literals.get(0).getOpposite());
							else
								lSet.add(r.literals.get(j));
						}
					}
					else
					{
						lSet.add(r.literals.get(0));
						lSet.add(r.literals.get(i).getOpposite());
					}
					tmpCnfs.add(new Clause(lSet));
				}
			}
		}

		Set<Clause> cs = new LinkedHashSet<>(tmpCnfs);
		clauses.addAll(tmpCnfs);
		return "";
	}

	private static String convertQuery(Query q, boolean isnot)
	{
		int count = q.at.symbolCount;
		querySymbols = q.at.symbols;
		queryClause = new LinkedHashSet<>(); 
		long bitsMax = (long) Math.pow(2,count);
		for (int i = 0; i < bitsMax; i++) {
			q.at.setTFtable(i);
			if(q.solve() == isnot)//~a is true means a is false => cnf!
			{
				int bit = (int) i;
				clauses.add(q.at.getClause(bit));
				queryClause.add(q.at.getClause(bit));
			}
		}
		return "";
	}
	private void usage() {

		System.out.println("Usage:\n\n-i map eg gameboard.txt");
		System.out.println("-o1 the file that writes results of KB based on map. eg KB.txt");
		System.out.println("-o2 the file that writes results of queries. eg.results.txt");
		System.out.println("-q the inut file of queries. eg query.txt");
		System.out.println("-ws worldsize eg. 4");
		System.out.println("-h algorithms 1 for resolution and 2 for walksat");

	}

	private int readPara(String args[]) {

		int n=0;

		for(int i=0;i<args.length;i++)
		{
			if(args[i].equals("-i"))
			{
				this.gameboard=args[i+1]; n++;
			}
			else if(args[i].equals("-q"))
			{	this.query=args[i+1]; n++;}
			else if(args[i].equals("-ws"))
			{	this.worldSize=Integer.parseInt(args[i+1]); n++;}
			else if(args[i].equals("-h"))
			{	this.algo=Integer.parseInt(args[i+1]); n++;}
			else if(args[i].equals("-o1"))
			{	this.KB=args[i+1]; n++;}
			else if(args[i].equals("-o2"))
			{	this.results=args[i+1]; n++;}
		}

		return n;
	}

	public static char[][] readWumpusWorld(int size, String gameboard) throws Exception {
		char[][] parsedWorld = new char[size][size];
		String gameBoardLine = "";
		File file = new File(gameboard);
		BufferedReader br = new BufferedReader(new FileReader(file));
		int column = 4;
		boolean goldAgentLine = false;

		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				parsedWorld[i][j] = 0;
			}
		}
		while((gameBoardLine = br.readLine())!= null)
		{
			if(gameBoardLine.contains("-"))
			{
				column--;
				continue;
			}
			for (int i = 0; i < 4; i++) 
			{
				if(gameBoardLine.length() <= 1)
					break;
				String str = gameBoardLine.substring(i*6+1, (i+1)*6);
				if(!goldAgentLine)
				{
					//parsing wumpus and pit
					if(str.contains("B"))
						parsedWorld[i][column] = 'B';
				}
			}
			goldAgentLine = !goldAgentLine;
		}
		br.close();

		return parsedWorld;
	}
}


class Learning{
	public static String[] studyKB(char wumpusWorld[][],int size, String KB, BufferedWriter bw)
	{ 
		String studyingKB = "";
		studyingKB = studyingKB+"B1,1<=>(P1,2VP2,1)!";
		studyingKB = studyingKB+"~B1,1!";
//		for (int i = 0; i < size; i++) 
//		{
//			for (int j = 0; j < size; j++) 
//			{
//				if(wumpusWorld[i][j] == 'B')
//					studyingKB = studyingKB + 'B'+i+","+j+"!";
//				else
//					studyingKB = studyingKB + "~B"+i+","+j+"!";
//				studyingKB = studyingKB + 'B'+i+","+j+"<=>";
//				String Pconjuct = "(";
//
//				if (i-1 >= 0) Pconjuct = Pconjuct + 'P'+(i-1)+","+j+"V";
//				if (i+1 < size) Pconjuct = Pconjuct + 'P'+(i+1)+","+j+"V";
//				if (j-1 >= 0) Pconjuct = Pconjuct + 'P'+i+","+(j-1)+"V";
//				if (j+1 < size) Pconjuct = Pconjuct + 'P'+i+","+(j+1)+"V";
//				String bicond = Pconjuct.substring(0,Pconjuct.lastIndexOf('V'));
//				bicond = bicond + ")!";
//				studyingKB = studyingKB + bicond;
//
//			}
//		}

		KB = studyingKB.substring(0,studyingKB.lastIndexOf('!'));
		String []kbs = KB.split("!");
		try {
		for (int i = 0; i < kbs.length; i++) {
			bw.write((i+1)+"."+kbs[i]);
			bw.newLine();
		}
		}catch (IOException e) {
			e.printStackTrace();
		}
		return kbs;
	}
}