require('dotenv').config();
const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');

const { connectDB, sequelize } = require('./config/database');
const logger = require('./config/logger');

const authRoutes = require('./routes/authRoutes');
const taskRoutes = require('./routes/taskRoutes');
const executionRoutes = require('./routes/executionRoutes');

const app = express();

// Middleware
app.use(helmet());   // WHY: Sets secure HTTP headers automatically
app.use(cors());     // WHY: Allows frontend/other services to call this API
app.use(morgan('dev'));
app.use(express.json());

// Routes
app.use('/api/auth', authRoutes);
app.use('/api/tasks', taskRoutes);
app.use('/api/executions', executionRoutes);

// Health check — used by GCP Cloud Run and CI/CD pipeline
app.get('/health', (req, res) => {
  res.json({ status: 'ok', service: 'task-scheduler-api', timestamp: new Date() });
});

// Global error handler
app.use((err, req, res, next) => {
  logger.error(err.stack);
  res.status(err.status || 500).json({ success: false, message: err.message });
});

const PORT = process.env.PORT || 3000;

async function bootstrap() {
  // Start the HTTP server FIRST
  app.listen(PORT, () => {
    logger.info(`API running on port ${PORT}`);
  });

  // Connect to database AFTER server starts
  try {
    await connectDB();
    await sequelize.sync({ alter: true });
    logger.info("Database connected successfully");
  } catch (err) {
    logger.error("Database connection failed:", err);
  }
}

bootstrap();

module.exports = app;
