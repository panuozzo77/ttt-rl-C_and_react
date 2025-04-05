import React, { useState, useEffect } from 'react';
import axios from 'axios';
import { Line } from 'react-chartjs-2';
import {
    Chart as ChartJS,
    CategoryScale,
    LinearScale,
    PointElement,
    LineElement,
    Title,
    Tooltip,
    Legend,
} from 'chart.js';
import './TrainingPage.css'; // Create this file

// Register Chart.js components
ChartJS.register(
    CategoryScale,
    LinearScale,
    PointElement,
    LineElement,
    Title,
    Tooltip,
    Legend
);

const API_URL = 'http://localhost:3001'; // Your backend URL

function TrainingPage() {
    const [chartData, setChartData] = useState(null);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState(null);

    useEffect(() => {
        const fetchData = async () => {
            setLoading(true);
            setError(null);
            try {
                const response = await axios.get(`${API_URL}/api/training-data`);
                const rawData = response.data; // Expecting array from backend

                if (!Array.isArray(rawData) || rawData.length === 0) {
                     throw new Error("No training data found or data is not an array.");
                }

                // --- Process data for charting ---
                const labels = rawData.map(entry => entry.games); // X-axis: number of games
                const winPercentages = rawData.map(entry => entry.win_pct);
                const lossPercentages = rawData.map(entry => entry.loss_pct);
                const tiePercentages = rawData.map(entry => entry.tie_pct);

                setChartData({
                    labels: labels,
                    datasets: [
                        {
                            label: 'AI Win %',
                            data: winPercentages,
                            borderColor: 'rgb(75, 192, 192)', // Greenish
                            backgroundColor: 'rgba(75, 192, 192, 0.5)',
                            tension: 0.1,
                            yAxisID: 'y', // Use the primary y-axis
                        },
                        {
                            label: 'AI Loss %',
                            data: lossPercentages,
                            borderColor: 'rgb(255, 99, 132)', // Reddish
                            backgroundColor: 'rgba(255, 99, 132, 0.5)',
                            tension: 0.1,
                            yAxisID: 'y',
                        },
                         {
                            label: 'Tie %',
                            data: tiePercentages,
                            borderColor: 'rgb(201, 203, 207)', // Grey
                            backgroundColor: 'rgba(201, 203, 207, 0.5)',
                            tension: 0.1,
                            yAxisID: 'y',
                        },
                        // Add more datasets if you logged other things like weights
                    ],
                });
                // --- End processing ---

            } catch (err) {
                console.error("Error fetching or processing training data:", err);
                 let errorMsg = 'Failed to load training data.';
                 if (err.response && err.response.data && err.response.data.error) {
                     errorMsg = `Server Error: ${err.response.data.error}`;
                 } else if (err.message) {
                     errorMsg = err.message;
                 }
                setError(errorMsg);
                setChartData(null); // Clear any previous data
            } finally {
                setLoading(false);
            }
        };
        fetchData();
    }, []); // Empty dependency array means run once on mount

    const chartOptions = {
        responsive: true,
        plugins: {
            legend: {
                position: 'top',
            },
            title: {
                display: true,
                text: 'AI Performance Over Training Games',
            },
            tooltip: {
                mode: 'index',
                intersect: false,
            },
        },
        scales: {
            x: {
                title: {
                    display: true,
                    text: 'Number of Training Games Played',
                },
            },
            y: { // Primary Y-axis for percentages
                type: 'linear',
                display: true,
                position: 'left',
                title: {
                    display: true,
                    text: 'Percentage (%)',
                },
                min: 0, // Optional: Set min/max for percentage axis
                max: 100,
            },
            // Add more y-axes if needed for other data types (e.g., weights)
        },
    };


    return (
        <div className="training-page">
            <h2>Training Progress Visualization</h2>
            {loading && <p>Loading training data...</p>}
            {error && <p className="error-message">Error: {error}</p>}
            {!loading && !error && !chartData && <p>No training data available to display.</p>}
            {chartData && (
                <div className="chart-container">
                    <Line options={chartOptions} data={chartData} />
                </div>
            )}
             {!loading && !error && (
                 <p className="data-source-note">
                     Data is read from the <code>training_log.json</code> file generated during the C program's training phase.
                 </p>
             )}
        </div>
    );
}

export default TrainingPage;