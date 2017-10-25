/* Authored by Kristen Patterson and Kenny Steinfeldt
 * October 27, 2017
 * Concurrency 2: The Dining Philosophers Problem
 */
import java.util.concurrent.ThreadLocalRandom;
import java.util.concurrent.Semaphore;

/*This program implements the Chandy/Misra solution
 * https://en.wikipedia.org/wiki/Dining_philosophers_problem#Chandy.2FMisra_solution
 */

//Class to represent chopsticks or forks
class Chopstick{
	private String mainuser;
	private Semaphore sem;
	private boolean clean;
	public int id;

	public Chopstick(int id, String user){
		this.mainuser = user;
		this.sem = new Semaphore(1);
		this.clean = false;
		this.id = id;
	}

	public void trylock(){
		try{
			sem.acquire();
		}catch (InterruptedException ex){
			Thread.currentThread().interrupt();
		}
	}

	public synchronized void grabChopstick(String user){
		while(this.mainuser != user){
			try{
				if(!clean){
					try{
						sem.acquire();
						this.unclean = false;
						System.out.println("Chopstick " 
								+ id + " is going from "
								+ mainuser + " to "
								+ user);
					} catch(InterruptedException ex){
						Thread.currentThread().interrupt();
					} finally{
						sem.release();
					}
				} else {
					System.out.println("Attempt to give Chopstick from "
							+ mainuser + " to " + user
							+ " but failed.");
				}
			} catch(InterruptedException ex){
				Thread.currentThread().interrupt();
			}
		}
	}

	public synchronized void setChopstick(){
		this.clean = false;
		notifyAll();
	}

	public void acq(){
		try{
			sem.acquire();
		}catch(InterruptedException ex){
			Thread.currentThread().interrupt();
		}
	}

	public void rel(){
		sem.release();
	}
}