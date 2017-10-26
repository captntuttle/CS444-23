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

	public synchronized void grabChopstick(String user){
		while(this.mainuser != user){
			try{
				if(!clean){
					try{
						sem.acquire();
						this.clean = true;
						System.out.println(user + " receives Chopstick " 
								+ id + " from "
								+ mainuser);
						this.mainuser = user;
					} catch(InterruptedException ex){
						Thread.currentThread().interrupt();
					} finally{
						sem.release();
					}
				} else {
					System.out.println("Attempt to give Chopstick from "
							+ mainuser + " to " + user
							+ " failed.");
					wait();
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

class Philosopher extends Thread{
	private String philo_name;
	public Chopstick rightChopstick;
	public Chopstick leftChopstick;

	public Philosopher(String philo_name, Chopstick rightChopstick, Chopstick leftChopstick){
		this.philo_name = philo_name;
		this.rightChopstick = rightChopstick;
		this.leftChopstick = leftChopstick;
	}

	public void think(){
		int rand;
		try{
			System.out.println(philo_name + ": thinking...");

			rand = (ThreadLocalRandom.current().nextInt(1, 20 + 1));
			rand = rand * 1000;
			Thread.sleep(rand);

			System.out.println(philo_name + ": finished thinking");

		}catch(InterruptedException ex){
			Thread.currentThread().interrupt();
		}
	}

	public void eat(){
		int rand;
		try{
			rightChopstick.acq();
			leftChopstick.acq();
			
			System.out.println(philo_name + ": eating...");

			rand = (ThreadLocalRandom.current().nextInt(2, 9 + 1));
			rand = rand * 1000;
			Thread.sleep(rand);

			rightChopstick.rel();
			leftChopstick.rel();

			System.out.println(philo_name + ": finished eating");
		}catch(InterruptedException ex){
			Thread.currentThread().interrupt();
		}
	}

	public void run(){
		while(true){
			think();
			rightChopstick.grabChopstick(philo_name);
			leftChopstick.grabChopstick(philo_name);
			eat();
			rightChopstick.setChopstick();
			leftChopstick.setChopstick();
		}
	}
}

public class philosophers{
	private Philosopher[] philos;
	public Chopstick[] chopsticks;
	//Why's it gotta be real people I coulda had fun and made a funny
	public String[] philo_names = {"Todd Howard", "Phil Spencer", "David Dague", "Hideo Kojima", "Shigeru Miyamoto"};

	public philosophers(){
		chopsticks = new Chopstick[5];
		philos = new Philosopher[5];
		int cid, setChopstick;

		for(int i = 0; i < chopsticks.length; i++){
			cid = 0;
			setChopstick = ((i + 4) % 5);
		
			if(i < setChopstick)
				cid = i;
			else
				cid = setChopstick;

			Chopstick chopstick = new Chopstick(i, philo_names[cid]);
			System.out.println(philo_names[cid] + " holds chopstick "
					+ i);
			chopsticks[i] = chopstick;
		}

		for(int j = 0; j < philos.length; j++){
			philos[j] = new Philosopher(philo_names[j], chopsticks[j], chopsticks[(j + 1) % 5]);
			philos[j].start();
		}
	}

	public static void main(String[] args){
		new philosophers();
	}
}
