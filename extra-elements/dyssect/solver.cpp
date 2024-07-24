#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gurobi_c++.h"

#include <sstream>
#include <deque>

#include <cmath>
#include <ctime>
#include <fcntl.h>
#include <cstdint>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#define solver_IN "./solver_IN"
#define solver_OUT "./solver_OUT"

uint32_t SCALE;

using namespace std;

static inline int from_pipe(int fd, uint8_t* addr, int len) 
{
        int n;
        int i = 0;

        while(i != len) 
	{
                n = read(fd, addr + i, len - i);
                if(n <= 0) 
		{
                        return i;
		}

                i += n;
        }

        return i;
}

static inline int to_pipe(int fd, uint8_t* addr, int len) 
{
        int n;
        int i = 0;

        while(i != len) 
	{
                n = write(fd, addr + i, len - i);
                if(n <= 0)
		{
                        return i;
		}

                i += n;
        }

        return i;
}

inline
void long_solver(int fdIN) 
{
	int __attribute__((unused)) ret;

	uint32_t MAXCORES;
	uint32_t SHARDS;
	ret = read(fdIN, &MAXCORES, sizeof(uint32_t));
	ret = read(fdIN, &SHARDS, sizeof(uint32_t));

	double Cap, Csp, SLOp;
	ret = read(fdIN, &Cap,  sizeof(double));
	ret = read(fdIN, &Csp,  sizeof(double));
	ret = read(fdIN, &SLOp, sizeof(double));

	double Car, Csr, SLOr;
	ret = read(fdIN, &Car,  sizeof(double));
	ret = read(fdIN, &Csr,  sizeof(double));
	ret = read(fdIN, &SLOr, sizeof(double));

	SCALE = 0.4;
	SLOr *= 1e9;
	SLOp *= 1e9;

	double T;
	ret = read(fdIN, &T, sizeof(double));

	double Te;
	ret = read(fdIN, &Te, sizeof(double));

	T  *= 1e9;
	Te *= 1e9;
	
	double V[SHARDS];
	ret = from_pipe(fdIN, (uint8_t*) V, SHARDS * sizeof(double));

	uint32_t Aold[SHARDS * MAXCORES];
        uint32_t Oold[MAXCORES * MAXCORES];
        ret = from_pipe(fdIN, (uint8_t*) Aold, SHARDS * MAXCORES        * sizeof(uint32_t));
        ret = from_pipe(fdIN, (uint8_t*) Oold, MAXCORES * MAXCORES      * sizeof(uint32_t));

	close(fdIN);
	uint32_t vsize = SHARDS * MAXCORES + MAXCORES * MAXCORES + SHARDS;

	double lb1[vsize];
	double lb0[vsize];
	double ub1[vsize];
	double ub09[vsize];
	for(uint32_t i = 0; i < vsize; i++) 
	{
		lb0[i] =  0.0;
		lb1[i] = -1.0;
		ub1[i] = +1.0;
		ub09[i] = +0.9;
	}

	try 
	{
		//Creating the environment
		GRBEnv env = GRBEnv(false);
		env.start();
                      
		//Creating the model
		GRBModel model = GRBModel(env);
			
		//Setting up the model
		model.set(GRB_IntParam_Threads, 10);
		model.set(GRB_IntParam_OutputFlag, 0);
		model.set(GRB_IntParam_MIPFocus, 1);

		//Setting the NonConvex mode
		model.set(GRB_IntParam_NonConvex, 2);

		//Creating the variables
		GRBVar r_factor = model.addVar(0.0, 1.0, 1.0, GRB_BINARY);

		GRBVar *w = model.addVars(MAXCORES, GRB_BINARY);
		GRBVar *e = model.addVars(MAXCORES, GRB_BINARY);
		GRBVar *A = model.addVars(SHARDS * MAXCORES, GRB_BINARY);
		GRBVar *O = model.addVars(MAXCORES * MAXCORES, GRB_BINARY);
			
		GRBVar *temp_w1 = model.addVars(lb1, ub1, NULL, NULL, NULL, NULL, MAXCORES);
		GRBVar *temp_w2 = model.addVars(lb1, ub1, NULL, NULL, NULL, NULL, MAXCORES);
		GRBVar *temp_e1 = model.addVars(lb1, ub1, NULL, NULL, NULL, NULL, MAXCORES);

		GRBVar *r = model.addVars(lb0, ub1, NULL, NULL, NULL, NULL, SHARDS);
		GRBVar *load_w  = model.addVars(lb0, ub09, NULL, NULL, NULL, NULL, MAXCORES);
		GRBVar *load_e  = model.addVars(lb0, ub09, NULL, NULL, NULL, NULL, MAXCORES);

		GRBVar *vars_w = model.addVars(MAXCORES);
		GRBVar *vars_e = model.addVars(MAXCORES);
		GRBVar *avg_w = model.addVars(MAXCORES);
		GRBVar *avg_e = model.addVars(MAXCORES);

		//Setting the model to minimize
		model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);

		GRBLinExpr W = 0;
		GRBLinExpr E = 0;
		for(uint32_t i = 0; i < MAXCORES; i++) 
		{
			W += w[i];
			E += e[i];
		}
		
		GRBLinExpr CORES = W + E;

		//Setting the objective
		GRBLinExpr obj = CORES;
		model.setObjective(obj);

		model.addConstr(W, GRB_GREATER_EQUAL, 1);

		model.addConstr(CORES, GRB_LESS_EQUAL, MAXCORES);

		model.addConstr(E, GRB_LESS_EQUAL, W/2);

		model.addGenConstrIndicator(e[0], 1, r_factor, GRB_EQUAL, 1);
		model.addGenConstrIndicator(e[0], 0, r_factor, GRB_EQUAL, 0);
		
		for(uint32_t s = 0; s < SHARDS; s++) 
		{
			model.addQConstr(r[s] * r_factor, GRB_EQUAL, r[s]);
		}

		for(uint32_t s = 0; s < SHARDS; s++) 
		{
			GRBLinExpr lhs = 0;
			for(uint32_t i = 0; i < MAXCORES; i++)
			{
				lhs += A[s*MAXCORES + i];
			}
			
			model.addConstr(lhs, GRB_EQUAL, 1);
		}

		GRBVar *res = model.addVars(MAXCORES);

		for(uint32_t i = 0; i < MAXCORES; i++) 
		{
			GRBLinExpr lhs = 0;
			GRBLinExpr rhs = 0;
			for(uint32_t s = 0; s < SHARDS; s++)
			{
				lhs += A[s*MAXCORES + i];
			}
			for(uint32_t j = 0; j < MAXCORES; j++)
			{
				rhs += O[j*MAXCORES + i];
			}

			GRBVar vars[2];
			vars[0] = (w[i]);
			vars[1] = (e[0]);

			model.addGenConstrAnd(res[i], vars, 2);

			model.addGenConstrIndicator(w[i], 0, lhs, GRB_EQUAL, 0);
			model.addGenConstrIndicator(w[i], 0, rhs, GRB_EQUAL, 0);
			model.addGenConstrIndicator(res[i], 1, rhs, GRB_EQUAL, 1);
			model.addGenConstrIndicator(w[i], 1, lhs, GRB_GREATER_EQUAL, 1);
		}

		for(uint32_t j = 0; j < MAXCORES; j++) 
		{
			GRBLinExpr lhs = 0;
			for(uint32_t i = 0; i < MAXCORES; i++)
			{
				lhs += O[j*MAXCORES + i];
			}

			model.addGenConstrIndicator(e[j], 0, lhs, GRB_EQUAL, 0);
			model.addGenConstrIndicator(e[j], 1, lhs, GRB_GREATER_EQUAL, 1);
		}

		for(uint32_t i = 0; i < MAXCORES-1; i++) 
		{
			GRBLinExpr lhs = 0;
			GRBLinExpr rhs = 0;
			for(uint32_t s = 0; s < SHARDS; s++) 
			{
				lhs += A[s*MAXCORES + i];
				rhs += A[s*MAXCORES + (i+1)];
			}
                        
			model.addConstr(lhs, GRB_GREATER_EQUAL, rhs);
			model.addConstr(w[i], GRB_GREATER_EQUAL, w[i+1]);
			model.addConstr(e[i], GRB_GREATER_EQUAL, e[i+1]);
		}

		for(uint32_t i = 0; i < MAXCORES; i++) 
		{
			GRBQuadExpr lhs = 0;
			GRBQuadExpr rhs = 0;
			
			for(uint32_t s = 0; s < SHARDS; s++) 
			{
				lhs += A[s*MAXCORES + i] * V[s] * (1.0-r[s]);
				rhs += A[s*MAXCORES + i] * V[s] * (r[s]);
			}

			model.addQConstr(temp_w1[i], GRB_EQUAL, lhs);
			model.addQConstr(temp_w2[i], GRB_EQUAL, rhs);
			model.addQConstr(load_w[i], GRB_EQUAL, w[i] * temp_w1[i]);

			model.addQConstr(vars_w[i] * (1 - load_w[i]), GRB_EQUAL, load_w[i]);
			model.addQConstr(avg_w[i], GRB_EQUAL, vars_w[i] * ((Cap*Cap + Csp*Csp)/2) * T);

			model.addQConstr(avg_w[i], GRB_LESS_EQUAL, SLOp);
		}

		for(uint32_t j = 0; j < MAXCORES; j++) 
		{
			GRBQuadExpr lhs = 0;

			for(uint32_t i = 0; i < MAXCORES; i++)
			{
				lhs += (O[j*MAXCORES + i] * temp_w2[i]);
			}

			model.addQConstr(temp_e1[j], GRB_EQUAL, lhs);
			model.addQConstr(load_e[j], GRB_EQUAL, SCALE * e[j] * temp_e1[j]);

			model.addQConstr(vars_e[j] * (1 - load_e[j]), GRB_EQUAL, load_e[j]);
			model.addQConstr(avg_e[j], GRB_EQUAL, vars_e[j] * ((Car*Car + Csr*Csr)/2) * (Te));

			model.addQConstr(avg_e[j], GRB_LESS_EQUAL, SLOr);
		}

		model.optimize();
                        
		if(model.get(GRB_IntAttr_Status) == GRB_OPTIMAL) 
		{
			uint32_t workings = 0;
			uint32_t offloadings = 0;
			for(uint32_t i = 0; i < MAXCORES; i++) 
			{
				workings  += std::round(w[i].get(GRB_DoubleAttr_X));
				offloadings += std::round(e[i].get(GRB_DoubleAttr_X));
			}

			int fdOUT = open((const char*) solver_OUT, O_CREAT | O_WRONLY, 0777);
			if(fdOUT == -1) 
			{
				printf("error: %s\n", strerror(errno));
				return;
			}

			int value = 1;
			ret = write(fdOUT, &value, sizeof(int));
                          
			ret = write(fdOUT, &workings, sizeof(uint32_t));
			ret = write(fdOUT, &offloadings, sizeof(uint32_t));

			close(fdOUT);
		} else 
		{
			int fdOUT = open((const char*) solver_OUT, O_CREAT | O_WRONLY, 0777);
			if(fdOUT == -1) 
			{
				printf("error: %s\n", strerror(errno));
				return;
			}
				
			int value = 0;
			ret = write(fdOUT, &value, sizeof(int));
				
			close(fdOUT);
		}
	} catch(GRBException e) 
	{
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
	} catch(...) 
	{
		cout << "Exception during optimization" << endl;
	}
}

inline
void short_solver(int fdIN) 
{
	int __attribute__((unused)) ret;

	uint32_t W;
	uint32_t E;
	uint32_t MAXCORES;
	uint32_t SHARDS;
	ret = read(fdIN, &W, sizeof(uint32_t));
	ret = read(fdIN, &E, sizeof(uint32_t));
	ret = read(fdIN, &MAXCORES, sizeof(uint32_t));
	ret = read(fdIN, &SHARDS, sizeof(uint32_t));
                
	double Cap, Csp, SLOp;
	ret = read(fdIN, &Cap,  sizeof(double));
	ret = read(fdIN, &Csp,  sizeof(double));
	ret = read(fdIN, &SLOp, sizeof(double));

	double Car, Csr, SLOr;
	ret = read(fdIN, &Car,  sizeof(double));
	ret = read(fdIN, &Csr,  sizeof(double));
	ret = read(fdIN, &SLOr, sizeof(double));
                
	SLOr *= 1e9;
	SLOp *= 1e9;

	double T;
	ret = read(fdIN, &T, sizeof(double));

	double Te;
	ret = read(fdIN, &Te, sizeof(double));

	T *= 1e9;
	Te *= 1e9;

	double V[SHARDS];
	ret = from_pipe(fdIN, (uint8_t*) V, SHARDS * sizeof(double));

	double rold[SHARDS];
	ret = from_pipe(fdIN, (uint8_t*) rold, SHARDS * sizeof(double));

	uint32_t Aold[SHARDS * MAXCORES];
	uint32_t Oold[MAXCORES * MAXCORES];
	ret = from_pipe(fdIN, (uint8_t*) Aold, SHARDS * MAXCORES        * sizeof(uint32_t));
	ret = from_pipe(fdIN, (uint8_t*) Oold, MAXCORES * MAXCORES      * sizeof(uint32_t));

	close(fdIN);
		
	uint32_t vsize = SHARDS * MAXCORES + MAXCORES * MAXCORES;
                
	double lb1[vsize];
	double lb0[vsize];
	double ub1[vsize];
	double ub09[vsize];
	for(uint32_t i = 0; i < vsize; i++) 
	{
		lb0[i] =  0.0;
		lb1[i] = -1.0;
		ub1[i] = +1.0;
		ub09[i] = +0.9;
	}

	try 
	{
		//Create the environment
		GRBEnv env = GRBEnv(false);
		env.start();
                        
		//Create the model
		GRBModel model = GRBModel(env);
                        
		//Set up the model
		model.set(GRB_IntParam_Threads, 10);
		model.set(GRB_IntParam_OutputFlag, 0);
		model.set(GRB_IntParam_MIPFocus, 1);
                        
		//Setting the 1 second time limit
		model.set(GRB_DoubleParam_TimeLimit, 0.1);

		//Setting the NonConvex mode
		model.set(GRB_IntParam_NonConvex, 2);
                        
		//Create the variables
		GRBVar *A = model.addVars(SHARDS * MAXCORES, GRB_BINARY);
		GRBVar *O = model.addVars(MAXCORES * MAXCORES, GRB_BINARY);

		GRBVar *r = model.addVars(lb0, ub1, NULL, NULL, NULL, NULL, SHARDS);
		GRBVar *load_w  = model.addVars(lb0, ub09, NULL, NULL, NULL, NULL, MAXCORES);
		GRBVar *load_e  = model.addVars(lb0, ub09, NULL, NULL, NULL, NULL, MAXCORES);
		GRBVar *load_we  = model.addVars(lb0, ub09, NULL, NULL, NULL, NULL, MAXCORES);
	
		GRBVar *vars_w = model.addVars(MAXCORES);
		GRBVar *vars_e = model.addVars(MAXCORES);
		GRBVar *avg_w = model.addVars(MAXCORES);
		GRBVar *avg_e = model.addVars(MAXCORES);

		//Setting the model to minimize
		model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);

		//Setting the objective
		GRBQuadExpr obj = 0;
                        
		GRBVar *diff = model.addVars(lb1, ub1, NULL, NULL, NULL, NULL, vsize);
		GRBVar *temp = model.addVars(lb1, ub1, NULL, NULL, NULL, NULL, vsize);

		uint32_t i = 0;

		for(uint32_t j = 0; j < SHARDS * MAXCORES; j++) 
		{
			model.addConstr(temp[i], GRB_EQUAL, (A[j] - Aold[j]));
			model.addGenConstrAbs(diff[i], temp[i]);
                                
			obj += (diff[i]);
			i++;
		}
		
		for(uint32_t j = 0; j < MAXCORES * MAXCORES; j++) 
		{
			model.addConstr(temp[i], GRB_EQUAL, (O[j] - Oold[j]));
			model.addGenConstrAbs(diff[i], temp[i]);
                                
			obj += (diff[i]);
			i++;
		}

		model.setObjective(obj);

		if(!E) 
		{
			for(uint32_t i = 0; i < MAXCORES; i++) 
			{
				GRBLinExpr lhs = 0;
				for(uint32_t j = 0; j < MAXCORES; j++)
				{
					lhs += O[j*MAXCORES + i];
				}
				model.addConstr(lhs, GRB_EQUAL, 0);
			}
			for(uint32_t s = 0; s < SHARDS; s++)
			{
				model.addConstr(r[s], GRB_EQUAL, 0);
			}
		} else 
		{
			for(uint32_t i = 0; i < W; i++) 
			{
				GRBLinExpr lhs1 = 0;
				GRBLinExpr lhs2 = 0;
				for(uint32_t j = 0; j < E; j++)
				{
					lhs1 += O[j*MAXCORES + i];
				}
				for(uint32_t j = E; j < MAXCORES; j++)
				{
					lhs2 += O[j*MAXCORES + i];
				}
                                        
				model.addConstr(lhs1, GRB_EQUAL, 1);
				model.addConstr(lhs2, GRB_EQUAL, 0);
			}

			for(uint32_t i = W; i < MAXCORES; i++) 
			{
				GRBLinExpr lhs = 0;
				for(uint32_t j = 0; j < MAXCORES; j++)
				{
					lhs += O[j*MAXCORES + i];
				}
                                        
				model.addConstr(lhs, GRB_EQUAL, 0);
			}

			for(uint32_t j = 0; j < E; j++) 
			{
				GRBLinExpr lhs = 0;
				for(uint32_t i = 0; i < MAXCORES; i++) 
				{
					lhs += O[j*MAXCORES + i];
				}

				model.addConstr(lhs, GRB_GREATER_EQUAL, 1);
			}
		}
 
		for(uint32_t s = 0; s < SHARDS; s++) 
		{
			GRBLinExpr lhs = 0;
			for(uint32_t i = 0; i < MAXCORES; i++)
			{
				lhs += A[s*MAXCORES + i];
			}
                                
			model.addConstr(lhs, GRB_EQUAL, 1);
		}

		for(uint32_t i = 0; i < W; i++) 
		{
			GRBLinExpr lhs = 0;
			for(uint32_t s = 0; s < SHARDS; s++) 
			{
				lhs += A[s*MAXCORES + i];
			}

			model.addConstr(lhs, GRB_GREATER_EQUAL, 1);
		}

		for(uint32_t i = W; i < MAXCORES; i++) 
		{
			GRBLinExpr lhs = 0;
			for(uint32_t s = 0; s < SHARDS; s++) 
			{
				lhs += A[s*MAXCORES + i];
			}

			model.addConstr(lhs, GRB_EQUAL, 0);
		}
	
		for(uint32_t i = 0; i < MAXCORES-1; i++) 
		{
			GRBLinExpr lhs = 0;
			GRBLinExpr rhs = 0;
			for(uint32_t s = 0; s < SHARDS; s++) 
			{
				lhs += A[s*MAXCORES + i];
				rhs += A[s*MAXCORES + (i+1)];
			}
                                
			model.addConstr(lhs, GRB_GREATER_EQUAL, rhs);
		}

		for(uint32_t i = 0; i < MAXCORES; i++) 
		{
			GRBQuadExpr lhs = 0;
			GRBQuadExpr rhs = 0;
                                
			for(uint32_t s = 0; s < SHARDS; s++) 
			{
				lhs += A[s*MAXCORES + i] * (V[s]) * (1.0-r[s]);
				rhs += A[s*MAXCORES + i] * (V[s]) * (r[s]);
			}
                               
			model.addQConstr(lhs, GRB_EQUAL, load_w[i]);
			model.addQConstr(rhs, GRB_EQUAL, load_we[i]);
                       
			model.addQConstr(vars_w[i] * (1 - load_w[i]), GRB_EQUAL, load_w[i]);
			model.addQConstr(avg_w[i], GRB_EQUAL, vars_w[i] * ((Cap*Cap + Csp*Csp)/2) * T);

			model.addQConstr(avg_w[i], GRB_LESS_EQUAL, SLOp);
		}
		
		for(uint32_t j = 0; j < MAXCORES; j++) 
		{
			GRBQuadExpr lhs = 0;
                                
			for(uint32_t i = 0; i < MAXCORES; i++)
			{
				lhs += (O[j*MAXCORES + i] * load_we[i]);
			}
                                
			model.addQConstr(SCALE * lhs, GRB_EQUAL, load_e[j]);
                                
			model.addQConstr(vars_e[j] * (1 - load_e[j]), GRB_EQUAL, load_e[j]);
			model.addQConstr(avg_e[j], GRB_EQUAL, vars_e[j] * ((Car*Car + Csr*Csr)/2) * (Te));

			model.addQConstr(avg_e[j], GRB_LESS_EQUAL, SLOr);
		}
		
		model.optimize();
		
		if(model.get(GRB_IntAttr_Status) == GRB_OPTIMAL) 
		{
			int fdOUT = open((const char*) solver_OUT, O_CREAT | O_WRONLY, 0777);
			if(fdOUT == -1) 
			{
				printf("error: %s\n", strerror(errno));
				return;
			}

			int value = 1;
			ret = write(fdOUT, &value, sizeof(int));
                                
			//r
			double newr[SHARDS];
			for(uint32_t s = 0; s < SHARDS; s++)
			{
				newr[s] = r[s].get(GRB_DoubleAttr_X);
			}
                                
			//A
			uint32_t newA[SHARDS * MAXCORES];
			for(uint32_t s = 0; s < SHARDS; s++) 
			{
				for(uint32_t i = 0; i < MAXCORES; i++) 
				{
					newA[s*MAXCORES + i] = (uint32_t) std::round(A[s*MAXCORES + i].get(GRB_DoubleAttr_X));
				}
			}
                                
			//O
			uint32_t newO[MAXCORES * MAXCORES];
			for(uint32_t i = 0; i < MAXCORES; i++) 
			{
				for(uint32_t j = 0; j < MAXCORES; j++) 
				{
					newO[j*MAXCORES + i] = (uint32_t) std::round(O[j*MAXCORES + i].get(GRB_DoubleAttr_X));
				}
			}
		
			ret = to_pipe(fdOUT, (uint8_t*) newr, SHARDS * sizeof(double));
			ret = to_pipe(fdOUT, (uint8_t*) newA, SHARDS * MAXCORES * sizeof(uint32_t));
			ret = to_pipe(fdOUT, (uint8_t*) newO, MAXCORES * MAXCORES * sizeof(uint32_t));

			close(fdOUT);
		} else 
		{
			int fdOUT = open((const char*) solver_OUT, O_CREAT | O_WRONLY, 0777);
			if(fdOUT == -1) 
			{
				printf("error: %s\n", strerror(errno));
				return;
			}

			int value = 0;
			ret = write(fdOUT, &value, sizeof(int));
                                
			close(fdOUT);
		}
	} catch(GRBException e) 
	{
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
	} catch(...) 
	{
		cout << "Exception during optimization" << endl;
	}
}

int main(int argc, char **argv) 
{
        int __attribute__((unused)) ret;
        
	int status = mkfifo(solver_IN, 0755);
        if(status < 0) 
	{
                unlink(solver_IN);
                status = mkfifo(solver_IN, 0755);
        }

	while(1) 
	{
		int fdIN = open((const char*) solver_IN, O_RDONLY);
                if(fdIN == -1) 
		{
			return -1;
		}

		uint32_t mode;

		ret = read(fdIN, &mode, sizeof(uint32_t));
		if(mode == 0) 
		{
			long_solver(fdIN);
		} else 
		{
			short_solver(fdIN);
		}
	}

	return 0;
}
