﻿namespace chess
{
    public interface Piece
    {
        Square Position { get; set; }
        string Color { get; set; }
        string Content { get; set; }
        void Move(Square target);
        bool IsAvailableMove(Square target);
        void Capture(Piece target);
        void Remove();
    }
}
