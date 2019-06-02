﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace chess
{
    class Rook : Piece
    {
        public Rook(Square position, string color)
        {
            Position = position;
            Color = color;
            Content = Color == "White" ? "\u2656" : "\u265C";
        }
        public Square Position { get; set; }
        public string Content { get; set; }
        public string Color { get; set; }

        public bool CanCapture(Square target)
        {
            throw new NotImplementedException();
        }

        public void Capture(Square target)
        {
            throw new NotImplementedException();
        }

        public bool IsAvailableMove(Square target)
        {
            throw new NotImplementedException();
        }

        public void Move(Square target)
        {
            throw new NotImplementedException();
        }

        public void Remove()
        {
            throw new NotImplementedException();
        }
    }
}
