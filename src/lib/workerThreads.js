import { Worker, isMainThread, workerData, parentPort, threadId } from 'worker_threads';

let instance = null;
let tasks = {};

export class WorkerThreads {
  constructor(poolSize, scriptPath) {
    this.scriptPath = scriptPath;
    this.poolSize = poolSize;
    this.workers = [];
    this.workerPromises = [];

    if(isMainThread) {
      //this.initWorkers();
    }

    instance = this;
  }

  /**
   * @return {WorkerThreads}
   */
  static getInstance() {
    if (!instance) {
      throw new Error("WorkerThreads instance not initialized");
    }
    return instance;
  }

  initWorkers() {
    console.log("Init Workers");
    for (let i = 0; i < this.poolSize; i++) {
      const worker = new Worker(this.scriptPath, { workerData: { id: i } });
      worker.on('error', (error) => {
        console.error(`Worker ${i} error:`, error);
      });
      worker.on('exit', (code) => {
        console.log(`Worker ${i} stopped with exit code ${code}`);
      });
      worker.on('message', (message) => {
        if(this.workerPromises[i]) {
          const p = this.workerPromises[i];
          this.workerPromises[i] = undefined;
          p.resolve(message);
        } else {
          console.warn(`Worker ${i} sent a message but no promise was waiting.`);
        }
      });
      worker.on('error', (error) => {
        console.error(`Worker ${i} error:`, error);
      });

      this.workers.push(worker);
      this.workerPromises.push(undefined);
    }
  }

  runTask(type, data) {
    let freeWorker = this.workerPromises.indexOf(undefined);
    if(freeWorker === -1) {
      console.log(this.workerPromises);
      throw new Error("No free workers available");
    }

    //console.log(`Using worker ${freeWorker}`);

    const worker = this.workers[freeWorker];
    return new Promise((resolve, reject) => {
      this.workerPromises[freeWorker] = { resolve, reject };
      worker.postMessage({ type, data });
    });
  }

  stop() {
    this.workers.forEach(worker => worker.terminate());
  }
}

export function initWorker() {
  if (!isMainThread) {
    parentPort.on('message', async (message) => {
      const { type, data } = message;
      if(!tasks[type]) {
        throw new Error(`Task type "${type}" not registered`);
      }
      try {
        parentPort.postMessage(tasks[type](data));
      } catch (error) {
        console.error(error);
        parentPort.postMessage({ error: error.message });
      }
    });
    console.log(`Worker ${threadId} initialized`);
  }
  return !isMainThread;
}

export function registerTask(type, func) {
  tasks[type] = func;
}