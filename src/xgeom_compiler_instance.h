namespace xgeom_compiler
{
    struct instance
    {
        virtual void LoadRaw        ( const std::string_view FilePath ) = 0;
        virtual void Compile        ( const descriptor& Options ) = 0;
        virtual void Serialize      ( const std::string_view FilePath ) = 0;
    };

    std::unique_ptr<instance> MakeInstance();
}