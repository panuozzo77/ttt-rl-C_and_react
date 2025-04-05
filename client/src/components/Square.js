import React from 'react';
import './Square.css'; // Create this file

function Square({ value, onClick }) {
  const symbolClass = value === 'X' ? 'symbol-x' : value === 'O' ? 'symbol-o' : '';
  return (
    <button className={`square ${symbolClass}`} onClick={onClick}>
      {value}
    </button>
  );
}

export default Square;