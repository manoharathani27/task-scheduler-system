const express = require('express');
const router = express.Router();
const auth = require('../middleware/auth');
const {
  createTask, getAllTasks, getTask,
  updateTask, pauseTask, resumeTask, deleteTask
} = require('../controllers/taskController');

router.use(auth);  // All task routes require JWT
router.post('/', createTask);
router.get('/', getAllTasks);
router.get('/:id', getTask);
router.put('/:id', updateTask);
router.post('/:id/pause', pauseTask);
router.post('/:id/resume', resumeTask);
router.delete('/:id', deleteTask);

module.exports = router;
